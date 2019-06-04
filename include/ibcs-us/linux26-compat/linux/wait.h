/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/wait.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_WAIT_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_WAIT_H
#include <linux/wait.h>

#include <ibcs-us/linux26-compat/linux/list.h>

typedef struct __wait_queue wait_queue_t;
typedef int (*wait_queue_func_t)(wait_queue_t* wait, unsigned mode, int flags, void* key);

struct __wait_queue
{
    unsigned int	flags;
#define WQ_FLAG_EXCLUSIVE	0x01
    void*		private;
    wait_queue_func_t	func;
    struct list_head	task_list;
};

struct __wait_queue_head
{
    struct list_head task_list;
};
typedef struct __wait_queue_head wait_queue_head_t;

#define wake_up_interruptible(x)

#endif
