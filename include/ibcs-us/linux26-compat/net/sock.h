/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/sock.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_SOCK_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_SOCK_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/net.h>
#include <ibcs-us/linux26-compat/linux/socket.h>

struct inode;

extern struct socket* inode_get__i_socket(struct inode* inode);

static inline struct socket* SOCKET_I(struct inode* inode)
{
    return inode->_i_socket ? inode->_i_socket : inode_get__i_socket(inode);
}
#endif
