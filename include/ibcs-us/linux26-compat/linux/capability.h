/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/capability.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_CAPABILITY_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_CAPABILITY_H
#include <linux/capability.h>
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/types.h>

#define cap_raise(c, flag)  linux26_capability(flag, 1)
#define cap_lower(c, flag)  linux26_capability(flag, -1)
#define cap_raised(c, flag) linux26_capability(flag, 0)

extern int linux26_capability(unsigned int flag, int action);

static inline int capable(int cap)
{
    return linux26_capability(cap, 0) ? 0 : -EPERM;
}
#endif
