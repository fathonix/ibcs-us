/*
 * Copyright (c) 2008 Christian Lademann, ZLS Software GmbH <cal@zls.de>
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * Most emulations support 16 bit inodes only.  Linux has 32 bit inodes, so
 * we have to fake it by mapping 32 bit inodes to 16 bit ones.  This module
 * creates the mapping and remembers it.  16 bit inodes are reallocated on
 * an LRU basis, so it will all fall apart if uses more than 2^16 files
 * simultaneously.
 */
#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/ibcs/short-inode.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/rwsem.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/slab.h>
#include <ibcs-us/linux26-compat/linux/string.h>


#ifdef	UNIT_TEST
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>

#undef	abi_trace
#define	abi_trace(flg, msg, args...)	mock_abi_trace(flg, msg, args)
#define	down_write(s)			mock_down_write(s)
#define	ibcs_malloc(s)			malloc(s)
#define	ibcs_free(s)			free(s)
#define	init_rwsem(s)			mock_init_rwsem(s)
#define	up_write(s)			mock_up_write(s)


static void mock_abi_trace(int flag, const char* message, ...)
{
    va_list	args;
    va_start(args, message);
    vfprintf(stdout, message, args);
    va_end(args);
    fprintf(stdout, "\n");
}

static inline void mock_down_write(struct rw_semaphore* sem) {}
static inline void mock_init_rwsem(struct rw_semaphore* sem) {}
static inline void mock_up_write(struct rw_semaphore* sem) {}
#endif


/*
 * SHORT INODE MAPPING:
 * Keep a list of mappings from 32-bit inode numbers to 16-bit inode numbers.
 */
struct short_inode_element
{
    ino_t	ino;
    u_short	lru_prev;
    u_short	lru_next;
    u_short	hash_prev;
    u_short	hash_next;
};


/*
 * The inode number 0..SHORT_INODE_UNMAPPED inode numbers are never mapped.
 * (SHORT_INODE_UNMAPPED+1) onwards are mapped to 16 bit inode numbers.
 */
#define	SHORT_INODE_UNMAPPED	2
#define SHORT_INODE_MAX		(0x10000 - (SHORT_INODE_UNMAPPED + 1))
#define SHORT_INODE_UNUSED	((ino_t)-1)
#define SHORT_INODE_IS_UNUSED(x) ({ino_t n = (x); n == 0 || n == SHORT_INODE_UNUSED;})

static struct short_inode_element* short_inode_table;
static u_short		short_inode_oldest;
static struct rw_semaphore short_inode_sem;


/*
 * Initialise this inode entry if it hasn't been initiaised yet.
 *
 * The short_inode_table is memory mapped, so a page in it isn't allocated by
 * the kernel until we write to it.  Since most programs only open a few files
 * it's likely it won't use most of them, so if we avoid writing them till
 * the last moment it's likely the 786K we mmap()'ed is not at wasteful as it
 * seems.
 */
static void short_inode_lazy_init(u_short entry)
{
    if (short_inode_table[entry].ino != 0) {
	return;
    }
    short_inode_table[entry].ino = SHORT_INODE_UNUSED;
    short_inode_table[entry].lru_prev = (entry + SHORT_INODE_MAX - 1) % SHORT_INODE_MAX;
    short_inode_table[entry].lru_next = (entry + 1) % SHORT_INODE_MAX;
}


/*
 * Determine the 16-bit hash value for an inode number.
 */
static u_short short_inode_hashval(ino_t ino)
{
    u_short		h;

    h = (u_short)ino;
    if (ino > SHORT_INODE_MAX) {
	h ^= (u_short)(ino >> 16);
    }
    /*
     * short_inode_map() returns (index + SHORT_INODE_UNMAPPED + 1) to skip
     * special inode numbers 0, 1 and 2.  If we 'wrap' the calculated
     * hashvalue by -(SHORT_INODE_BASE + 1), we 16 bit inode numbers will
     * be an identity mapping - ie they won't change.
     */
    return (h + SHORT_INODE_MAX - SHORT_INODE_UNMAPPED - 1) % SHORT_INODE_MAX;
}


/*
 * Join two inodes in the lru chain.
 */
static inline void short_inode_lru_join(u_short prev, u_short next)
{
    short_inode_lazy_init(prev);
    short_inode_lazy_init(next);
    short_inode_table[prev].lru_next = next;
    short_inode_table[next].lru_prev = prev;
}


/*
 * Remove an inode from it's lru chain.
 */
static inline void short_inode_lru_free(u_short i)
{
    short_inode_lazy_init(i);
    short_inode_lru_join(
	short_inode_table[i].lru_prev, short_inode_table[i].lru_next);
}


/*
 * Join two inodes in the hash chain.
 */
static inline void short_inode_hash_join(u_short prev, u_short next)
{
    short_inode_table[prev].hash_next = next;
    short_inode_table[next].hash_prev = prev;
}


/*
 * Remove an inode from it's hash chain.
 */
static inline void short_inode_hash_free(u_short i)
{
    if (!SHORT_INODE_IS_UNUSED(short_inode_table[i].ino)) {
	short_inode_hash_join(
	    short_inode_table[i].hash_prev, short_inode_table[i].hash_next);
    }
}


/*
 * Map the passed inode to a 16 bit value.  Inode numbers 0, 1 and 2 are
 * always returned unchanged.  -ENOENT if returned if the mapping fails.
 */
ino_t short_inode_map(ino_t ino)
{
    u_short		si;

    if (!short_inode_table) {
	return ino & 0xffff ? ino : (ino_t)-ENOENT;
    }
    if (ino <= SHORT_INODE_UNMAPPED) {
	return ino;
    }
    down_write(&short_inode_sem);
    si = short_inode_hashval(ino);
    if (SHORT_INODE_IS_UNUSED(short_inode_table[si].ino)) {
	/*
	 * The hash slot used by the inode is unused, so just remove it from
	 * the free chain and allocate it.
	 */
	short_inode_lazy_init(si);
	if (short_inode_oldest == si) {
	    short_inode_oldest = short_inode_table[short_inode_oldest].lru_next;
	}
	short_inode_table[si].ino = ino;
	short_inode_hash_join(si, si);
    } else {
        /*
	 * The hash slot is used, so check if we are already on the hash chain.
	 */
	u_short first = si;
	do {
	    si = short_inode_table[si].hash_next;
	} while (short_inode_table[si].ino != ino && si != first);
	if (short_inode_table[si].ino == ino) {
	    /*
	     * We were on the hash chain - so move us to the end of the
	     * least recently used list (ie, we are now the most recently
	     * used).
	     */
	    short_inode_lru_free(si);
	    short_inode_lru_join(short_inode_table[short_inode_oldest].lru_prev, si);
	    short_inode_lru_join(si, short_inode_oldest);
	} else {
	    /*
	     * We weren't on the hash chain, so grab the least recently used
	     * entry, free it off the chain it's on, and allocate it to us.
	     */
	    short_inode_hash_free(short_inode_oldest);
	    short_inode_lazy_init(short_inode_oldest);
	    short_inode_hash_join(short_inode_oldest, short_inode_table[si].hash_next);
	    short_inode_hash_join(si, short_inode_oldest);
	    si = short_inode_oldest;
	    short_inode_table[si].ino = ino;
	    short_inode_oldest = short_inode_table[short_inode_oldest].lru_next;
	}
    }
    up_write(&short_inode_sem);
    ino_t mapped =(ino_t)si + SHORT_INODE_UNMAPPED + 1;
    abi_trace(
	ABI_TRACE_SINODE,
	"short_inode_map %x->%d(%d), next=%x->%d prev=%x->%d, oldest=%x->%d, old next=%x->%d old prev=%x->%d",
	(u_int)ino, si, mapped,
	short_inode_table[short_inode_table[si].hash_prev].ino,
	short_inode_table[si].hash_prev,
	short_inode_table[short_inode_table[si].hash_next].ino,
	short_inode_table[si].hash_next,
	short_inode_table[short_inode_oldest].ino, short_inode_oldest,
	short_inode_table[short_inode_table[short_inode_oldest].hash_prev].ino,
	short_inode_table[short_inode_oldest].hash_prev,
	short_inode_table[short_inode_table[short_inode_oldest].hash_next].ino,
	short_inode_table[short_inode_oldest].hash_next);
    return mapped;
}


int short_inode_construct(void)
{
    /*
     * There is a *BIG* implicit assumption here: the memory returned by
     * ibcs_malloc() will be entirely full of 0's.  If it isn't this code
     * will break.  The assumption is safesonable because this is enormous
     * chunk of memory is one of the first things allocoated and so
     * ibcs_malloc() will have to ask the kernel for it.
     *
     * We are doing this on the basis of an even bigger assumption: the
     * kernel won't actually bother to allocate the memory.  It will just
     * reserve the pages.  The pages will be lazily allocated by the kernel
     * (and of course zero filled) when we first reference them.  So
     * effectively we aren't really allocating the memory, we are just
     * reserving an address range.  It's very rare for a program to use more
     * more than just a few inodes, so the large number passed to
     * ibcs_malloc() notwithstanding this module won't use a lot of memory
     * unless it actually needs it.
     *
     * To ensure the memory isn't allocated we have to avoid reading or
     * writing it until absolutely necessary.  Thus we can't even zero it
     * out: we just have to trust it will all work the way we think it
     * does.
     */
    int short_inode_table_len = SHORT_INODE_MAX * sizeof(*short_inode_table);
    short_inode_table = (struct short_inode_element*)ibcs_malloc(
	short_inode_table_len);
    short_inode_oldest = 0;
    init_rwsem(&short_inode_sem);
    return 0;
}


void short_inode_destruct(void)
{
    if (short_inode_table == NULL) {
	return;
    }
    ibcs_free(short_inode_table);
    short_inode_table = NULL;
}

/*
 * Unit testing code.  This is run using "make tests" at the top level or
 * "make test-FILENAME" in the component directory.
 */
#ifdef	UNIT_TEST
#include <stdio.h>
#include <stdlib.h>

static int current_line_nr;
#define	MUST_BE_TRUE(line_nr, a)					\
    do { 								\
	current_line_nr = line_nr;					\
	if (!(a)) { 							\
	    fprintf(stderr, "%s line %d test failed, %s\n", __FILE__, line_nr, #a);	\
	    abort();							\
	}								\
    } while(0)


static void test_short_inode_map(int line_nr, ...)
{
    va_list		args;
    int			i;
    unsigned long	inode;
    int			inode_count;
    struct {long i; unsigned short s;} alloced[128];

    memset(alloced, '\0', sizeof(alloced));
    inode_count = 0;
    va_start(args, line_nr);
    inode = va_arg(args, int);
    int ret = short_inode_construct();
    MUST_BE_TRUE(line_nr, ret == 0);
    while (inode != 0) {
        unsigned short prev_oldest = short_inode_oldest;
        unsigned short next_alloc = prev_oldest + SHORT_INODE_UNMAPPED + 1;
	alloced[inode_count].i = inode;
	alloced[inode_count].s = short_inode_map(inode);
	MUST_BE_TRUE(line_nr, inode > 2 || alloced[inode_count].s == inode);
	for (i = 0; i < inode_count; i += 1) {
	    if (
		prev_oldest != short_inode_oldest &&
		next_alloc == alloced[inode_count].s &&
		next_alloc == alloced[i].s
	    ) {
		alloced[i].i = 0;
	    }
	    MUST_BE_TRUE(
		line_nr,
		alloced[i].i == 0 || alloced[i].s != alloced[inode_count].s);
	}
	inode_count += 1;
	inode = va_arg(args, int);
    }
    for (i = 0; i  < inode_count; i += 1) {
	MUST_BE_TRUE(
	    line_nr,
	    alloced[i].i == 0 || alloced[i].s == short_inode_map(alloced[i].i));
    }
    short_inode_destruct();
}

void unit_test_ibcs_short_inode_c()
{
#    define c(i,h)	((i ^ h) | (h << 16))
    test_short_inode_map(__LINE__, 1, 2, 3, c(3,1), c(3,2), 0);
    test_short_inode_map(__LINE__, 4, 9, c(9,1), c(9,2), c(9,3), 5, 0);
}
#endif
