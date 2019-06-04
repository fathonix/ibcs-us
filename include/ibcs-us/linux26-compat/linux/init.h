/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/init.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_INIT_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_INIT_H
#include <sys/cdefs.h>

#define	__init			__attribute__((constructor))
#define	__exit			__attribute__((destructor))
#define	module_init(x)
#define	module_exit(x)

#endif
