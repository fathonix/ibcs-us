/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/in.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_IN_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_IN_H

#include <linux/in.h>

#define ntohl(x)	({						\
			    u32 v = (x);				\
			    (typeof(x))(ntohs(v) << 16) | ntohs(v >> 16); \
			})

#define ntohs(x)	({						\
			    u16 v = (x);				\
			    (typeof(x))(v << 8) | (v >> 8);		\
			})

#define htonl(x)	ntohl(x)
#define htons(x)	ntohs(x)


#endif
