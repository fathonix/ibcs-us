/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/mm_types.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_MM_TYPES_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_MM_TYPES_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/asm/page.h>
#include <ibcs-us/linux26-compat/linux/rwsem.h>

struct mm_struct
{
    unsigned long	arg_end;
    unsigned long	arg_start;
    unsigned long	brk;
    unsigned long	end_code;
    unsigned long	end_data;
    unsigned long	env_end;
    unsigned long	env_start;
    struct rw_semaphore	mmap_sem;
    unsigned long	start_brk;
    unsigned long	start_code;
    unsigned long	start_data;
    unsigned long	start_stack;
    /*
     * Added by ibcs-us to implement brk().
     */
    unsigned long	ibcs_brk_mmap_addr;
    unsigned long	ibcs_brk_mmap_max;
    unsigned long	ibcs_brk_mmap_min;
    unsigned long	ibcs_bss_mmap_addr;
    unsigned long	ibcs_bss_mmap_len;
    unsigned long	ibcs_high_mmap_addr;
    unsigned long	ibcs_high_mmap_len;
};

#endif
