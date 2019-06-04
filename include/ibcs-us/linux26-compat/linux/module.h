/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/module.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_MODULE_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_MODULE_H
#include <sys/cdefs.h>

#define	EXPORT_NO_SYMBOLS
#define	EXPORT_SYMBOL(x)
#define	MODULE_AUTHOR(x)
#define	MODULE_DESCRIPTION(x)
#define	MODULE_INFO(x, y)
#define	MODULE_LICENSE(x)
#define	MODULE_PARM_DESC(x, y)
#define	THIS_MODULE		((struct module*)0)

#define	__init			__attribute__((constructor))
#define	__exit			__attribute__((destructor))
#define	module_init(x)
#define	module_exit(x)

/*
 * We don't have modules, so the only use is "THIS_MODULE" above.
 */
struct module;

#endif
