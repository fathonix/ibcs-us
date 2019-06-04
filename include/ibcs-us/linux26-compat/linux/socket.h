/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/socket.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_SOCKET_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_SOCKET_H
#include <stddef.h>
#include <sys/cdefs.h>

/*
 * Defines __socklen_t needed by the system include files.
 */
#include <ibcs-us/linux26-compat/linux/types.h>

#define	_SYS_SOCKET_H	1		/* <bits/socket.h> needs this. */
#define	_SYS_TYPES_H	1		/* Don't pull in glibc's <sys/types> */

#include <bits/socket.h>
#include <linux/socket.h>
#include <linux/sockios.h>

struct iovec
{
    void*		iov_base;	/* BSD uses caddr_t (1003.1g requires void *) */
    __kernel_size_t	iov_len; 	/* Must be size_t (1003.1g) */
};
 
#define UIO_FASTIOV	8
#define UIO_MAXIOV	1024

/* Setsockoptions(2) level. Thanks to BSD these must match IPPROTO_xxx */
#define SOL_IP		0
/* #define SOL_ICMP	1	No-no-no! Due to Linux :-) we cannot use SOL_ICMP=1 */
#define SOL_TCP		6
#define SOL_UDP		17
#define SOL_IPV6	41
#define SOL_ICMPV6	58
#define SOL_SCTP	132
#define SOL_UDPLITE	136     /* UDP-Lite (RFC 3828) */
#define SOL_RAW		255
#define SOL_IPX		256
#define SOL_AX25	257
#define SOL_ATALK	258
#define SOL_NETROM	259
#define SOL_ROSE	260
#define SOL_DECNET	261
#define	SOL_X25		262
#define SOL_PACKET	263
#define SOL_ATM		264	/* ATM layer (cell level) */
#define SOL_AAL		265	/* ATM Adaption Layer (packet level) */
#define SOL_IRDA        266
#define SOL_NETBEUI	267
#define SOL_LLC		268
#define SOL_DCCP	269
#define SOL_NETLINK	270
#define SOL_TIPC	271
#define SOL_RXRPC	272
#define SOL_PPPOL2TP	273
#define SOL_BLUETOOTH	274
#define SOL_PNPIPE	275
#define SOL_RDS		276
#define SOL_IUCV	277

#endif
