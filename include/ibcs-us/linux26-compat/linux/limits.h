/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/limits.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_LIMITS_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_LIMITS_H

/*
 * GCC's limits.h seems to be safe - no stuff only available in glibc
 * is defined.
 *
 * Note: the order of the includes matters here - don't change it!
 */
#include <linux/limits.h>
#include <limits.h>

#endif
