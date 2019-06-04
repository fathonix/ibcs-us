/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/rwsem.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_RWSEM_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_RWSEM_H
#include <sys/cdefs.h>

struct rw_semaphore
{
    int			rw_semaphore_futex;
};

/*
 * We don't bother with read/write - just a single exclusive lock implemented
 * with a futex.
 */
extern void down_read(struct rw_semaphore* sem);

static inline void down_write(struct rw_semaphore* sem)
{
    down_read(sem);
}

extern void up_read(struct rw_semaphore* sem);

static inline void up_write(struct rw_semaphore* sem)
{
    up_read(sem);
}

static inline void init_rwsem(struct rw_semaphore* rwsem)
{
    rwsem->rw_semaphore_futex = 1;
}

#endif
