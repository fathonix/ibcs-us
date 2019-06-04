/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <asm/processor.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_ASM_PROCESSOR_H
#define _IBCS_US_LINUX26_INCLUDE_ASM_PROCESSOR_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/asm/ptrace.h>

/*
 * User space process size. 47bits minus one guard page.
 */
#define TASK_SIZE_MAX		((1UL << 47) - PAGE_SIZE)

/* This decides where the kernel will search for a free chunk of vm
 * space during mmap's.
 */
#define IA32_PAGE_OFFSET	((current->personality & ADDR_LIMIT_3GB) ? \
					0xc0000000 : 0xFFFFe000)

#define TASK_SIZE		IA32_PAGE_OFFSET
#define TASK_SIZE_OF(child)	IA32_PAGE_OFFSET

#define STACK_TOP		TASK_SIZE
#define STACK_TOP_MAX		TASK_SIZE_MAX



typedef struct
{
    unsigned long	seg;
} mm_segment_t;

extern void start_thread(struct pt_regs* regs, unsigned long new_ip, unsigned long new_sp);
#endif
