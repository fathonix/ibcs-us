/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/mm.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_MM_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_MM_H
#include <stddef.h>
#include <sys/cdefs.h>

#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/linux26-compat/asm/mman.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/errno.h>

#define	PAGE_SHIFT	12
#define	PAGE_SIZE	(1UL << PAGE_SHIFT)
#define	PAGE_MASK	(~(PAGE_SIZE - 1))
#define	PAGE_ALIGN(x)	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

extern int		binfmt_mmap_errno;
extern const char*	binfmt_mmap_error;

extern unsigned long binfmt_mmap(
    struct file* file, unsigned long addr,
    unsigned long len, unsigned long prot,
    unsigned long flag, unsigned long offset);
extern unsigned long do_brk(unsigned long addr, unsigned long len);
extern unsigned long ibcs_brk(unsigned long addr);

static inline unsigned long do_mmap_pgoff(
    struct file *file, unsigned long addr,
    unsigned long len, unsigned long prot,
    unsigned long flag, unsigned long pgoff
) {
    if (!file) {
	flag |= MAP_ANONYMOUS;
    }
    return (unsigned long)IBCS_SYSCALL(
	  mmap2, (void*)addr, (size_t)len, (int)prot, (int)flag,
	  file ? file->fd : -1, (off_t)pgoff);
}

static inline unsigned long do_mmap(
    struct file *file, unsigned long addr,
    unsigned long len, unsigned long prot,
    unsigned long flag, unsigned long offset
) {
    if (offset + PAGE_ALIGN(len) < offset || offset & ~PAGE_MASK) {
	return -EINVAL;
    }
    return do_mmap_pgoff(file, addr, len, prot, flag, offset >> PAGE_SHIFT);
}


static inline int do_munmap(struct mm_struct* mm, unsigned long addr, size_t len)
{
    return IBCS_SYSCALL(munmap, (void*)addr, len);
}
#endif
