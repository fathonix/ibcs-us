/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/a.out.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_GFP_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_GFP_H
#include <stddef.h>
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/slab.h>
#include <ibcs-us/linux26-compat/linux/types.h>

#define GFP_KERNEL	0

/*
 * Page alignment probably matters in the kernel, but doesn't see to in user
 * space.
 */
static inline void* __get_free_page(unsigned flags)
{
    return kmalloc(PAGE_SIZE, 0);
}

static inline void free_page(unsigned long page)
{
    return kfree((void*)(ptrdiff_t)page);
}

#endif
