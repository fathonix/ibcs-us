/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/poll.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_POLL_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_POLL_H
#include <linux/poll.h>

#include <ibcs-us/linux26-compat/linux/string.h>
#include <ibcs-us/linux26-compat/linux/wait.h>

struct file;
struct poll_table_struct;

typedef void (*poll_queue_proc)(struct file*, wait_queue_head_t*, struct poll_table_struct*);

struct poll_table_struct
{
    poll_queue_proc	qproc;
    unsigned long	key;
};

typedef struct poll_table_struct poll_table;

/*
 * Structures and helpers for sys_poll/sys_poll
 */
struct poll_wqueues
{
    struct poll_table_struct pt;
};

static inline void poll_initwait(struct poll_wqueues* wq)
{
    memset(wq, 0, sizeof(*wq));
}

static inline void poll_freewait(struct poll_wqueues* wq)
{
}

extern int linux26_poll_wait(struct file* file, short poll_events);
#endif
