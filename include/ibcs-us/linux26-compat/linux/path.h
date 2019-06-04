/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/path.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_PATH_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_PATH_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/mount.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/statfs.h>

struct path
{
    struct vfsmount*	mnt;
    struct dentry*	dentry;
    struct vfsmount	_mnt_real;
    struct dentry	_dentry_real;
};

static inline void path_get(struct path* path) {}
static inline void path_put(struct path* path) {}

#endif
