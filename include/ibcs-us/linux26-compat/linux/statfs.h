/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/statfs.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_STATFS_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_STATFS_H
#include <asm/statfs.h>

#include <ibcs-us/linux26-compat/linux/types.h>

typedef __kernel_fsid_t	fsid_t;

struct kstatfs
{
    long		f_type;
    long		f_bsize;
    u64			f_blocks;
    u64			f_bfree;
    u64			f_bavail;
    u64			f_files;
    u64			f_ffree;
    __kernel_fsid_t	f_fsid;
    long		f_namelen;
    long		f_frsize;
    long		f_spare[5];
};

#endif
