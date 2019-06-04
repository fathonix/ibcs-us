/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/slab.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_SLAB_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_SLAB_H
#include <stddef.h>

#include <ibcs-us/linux26-compat/linux/types.h>

extern void kfree(const void* p);
extern void* kmalloc(size_t size, gfp_t flags);

#endif
