/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/stat.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_STAT_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_STAT_H
#include <sys/cdefs.h>
#include <linux/time.h>

#include <ibcs-us/linux26-compat/linux/types.h>

#include <asm/stat.h>

#ifdef	__KERNEL__
#include <linux/stat.h>
#else
#define	__KERNEL__
#include <linux/stat.h>
#undef	__KERNEL__
#endif

struct kstat
{
    u64			ino;
    dev_t		dev;
    umode_t		mode;
    unsigned int	nlink;
    uid_t		uid;
    gid_t		gid;
    dev_t		rdev;
    loff_t		size;
    struct timespec	atime;
    struct timespec	mtime;
    struct timespec	ctime;
    unsigned long	blksize;
    unsigned long long	blocks;
};

#endif
