/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/personality.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_PERSONALITY_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_PERSONALITY_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/sched.h>

/*
 * Flags for bug emulation.
 *
 * These occupy the top three bytes.
 */
enum
{
    ABI_Y2K_BUG=	0x0020000,	/* see include/ibcs-y2k.h */
    ADDR_NO_RANDOMIZE=	0x0040000,	/* disable randomization of VA space */
    FDPIC_FUNCPTRS=	0x0080000,	/* userspace function ptrs are signal descriptors */
    MMAP_PAGE_ZERO=	0x0100000,
    ADDR_COMPAT_LAYOUT=	0x0200000,
    READ_IMPLIES_EXEC=	0x0400000,
    ADDR_LIMIT_32BIT=	0x0800000,
    SHORT_INODE=	0x1000000,
    WHOLE_SECONDS=	0x2000000,
    STICKY_TIMEOUTS=	0x4000000,
    ADDR_LIMIT_3GB=	0x8000000,
};

/*
 * Security-relevant compatibility flags that must be
 * cleared upon setuid or setgid exec:
 */
#define PER_CLEAR_ON_SETID (READ_IMPLIES_EXEC  | \
			    ADDR_NO_RANDOMIZE  | \
			    ADDR_COMPAT_LAYOUT | \
			    MMAP_PAGE_ZERO)

/*
 * Personality types.
 *
 * These go in the low byte.  Avoid using the top bit, it will
 * conflict with error returns.
 */
enum
{
    PER_LINUX=		0x0000,
    PER_LINUX_32BIT=	0x0000 | ADDR_LIMIT_32BIT,
    PER_LINUX_FDPIC=	0x0000 | FDPIC_FUNCPTRS,
    PER_SVR4=		0x0001 | STICKY_TIMEOUTS | MMAP_PAGE_ZERO,
    PER_SVR3=		0x0002 | STICKY_TIMEOUTS | SHORT_INODE,
    PER_SCOSVR3=	0x0003 | STICKY_TIMEOUTS | WHOLE_SECONDS | SHORT_INODE,
    PER_OSR5=		0x0003 | STICKY_TIMEOUTS | WHOLE_SECONDS,
    PER_WYSEV386=	0x0004 | STICKY_TIMEOUTS | SHORT_INODE,
    PER_ISCR4=		0x0005 | STICKY_TIMEOUTS,
    PER_BSD=		0x0006,
    PER_SUNOS=		0x0006 | STICKY_TIMEOUTS,
    PER_XENIX=		0x0007 | STICKY_TIMEOUTS | SHORT_INODE,
    PER_LINUX32=	0x0008,
    PER_LINUX32_3GB=	0x0008 | ADDR_LIMIT_3GB,
    PER_IRIX32=		0x0009 | STICKY_TIMEOUTS,/* IRIX5 32-bit */
    PER_IRIXN32=	0x000a | STICKY_TIMEOUTS,/* IRIX6 new 32-bit */
    PER_IRIX64=		0x000b | STICKY_TIMEOUTS,/* IRIX6 64-bit */
    PER_RISCOS=		0x000c,
    PER_SOLARIS=	0x000d | STICKY_TIMEOUTS,
    PER_UW7=		0x000e | STICKY_TIMEOUTS | MMAP_PAGE_ZERO,
    PER_OSF4=		0x000f,			 /* OSF/1 v4 */
    PER_HPUX=		0x0010,
    PER_MASK=		0x00ff,
};

/*
 * Description of an execution domain.
 * 
 * The first two members are refernced from assembly source
 * and should stay where they are unless explicitly needed.
 */
struct pt_regs;
struct sysent;
struct module;

typedef void (*handler_t)(int lcall_vector, struct pt_regs* regs);

struct exec_domain
{
    const char*		name;		/* name of the execdomain */
    handler_t		handler;	/* handler for syscalls */
    unsigned char	pers_low;	/* lowest personality */
    unsigned char	pers_high;	/* highest personality */
    unsigned long*	signal_map;	/* signal mapping */
    unsigned long*	signal_invmap;	/* reverse signal mapping */
    struct sysent*	systable;	/* System call table */
    int			systable_size;	/* Number of entries in systable */
    int			syscall_offset;	/* Stack offset of syscalls */
    struct map_segment*	err_map;	/* error mapping */
    struct map_segment*	socktype_map;	/* socket type mapping */
    struct map_segment*	sockopt_map;	/* socket option mapping */
    struct map_segment*	af_map;		/* address family mapping */
    /* struct module*	module;		/ * module context of the ed. */
    struct exec_domain*	next;		/* linked list (internal) */
    struct module*	module;
};

/*
 * Return the base personality without flags.
 */
#define personality(pers)	(pers & PER_MASK)

extern int set_personality(u_long personality);
extern int register_exec_domain(struct exec_domain *);
extern int unregister_exec_domain(struct exec_domain *);
#endif
