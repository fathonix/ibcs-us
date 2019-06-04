/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/user.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_USER_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_USER_H
#include <sys/cdefs.h>

struct user_regs_struct
{
    unsigned long	ax;
    unsigned long	bp;
    unsigned long	bx;
    unsigned long	cs;
    unsigned long	cx;
    unsigned long	di;
    unsigned long	ds;
    unsigned long	dx;
    unsigned long	es;
    unsigned long	flags;
    unsigned long	fs;
    unsigned long	gs;
    unsigned long	ip;
    unsigned long	orig_ax;
    unsigned long	si;
    unsigned long	sp;
    unsigned long	ss;
};

struct user
{
    struct user_regs_struct regs;	/* Where the registers are actually stored */
    long int		signal;     	/* Signal that caused the core dump. */
    int			reserved;	/* No longer used */
};
#endif
