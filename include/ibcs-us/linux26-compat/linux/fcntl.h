/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/fcntl.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_FCNTL_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_FCNTL_H
#include <bits/types.h>
#include <sys/cdefs.h>

#define	_FCNTL_H			/* We don't want the libc fcntl.h */
#define __USE_LARGEFILE64
#include <bits/fcntl.h>
#include <bits/fcntl-linux.h>

#include <ibcs-us/linux26-compat/linux/mman.h>

#endif
