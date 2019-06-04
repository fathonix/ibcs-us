/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/net.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_NET_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_NET_H
#include <linux/net.h>
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/socket.h>

#define __SO_ACCEPTCON	(1 << 16)	/* performed a listen		*/

struct file*		file;

struct socket
{
    short		type;
    struct file*	file;
};

extern struct socket* sockfd_lookup(int fd, int* err);
extern void sockfd_put(struct socket* sock);
extern int sock_sendmsg(struct socket *sock, struct msghdr *msg, size_t len);
extern int sock_recvmsg(struct socket *sock, struct msghdr *msg, size_t size, int flags);

#endif
