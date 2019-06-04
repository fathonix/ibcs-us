/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/file.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_FILE_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_FILE_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/linux/fs.h>

static inline void fd_install(unsigned int fd, struct file* file) {}
extern struct file* fget(unsigned int fd);
extern void fput(struct file* file);
extern int get_unused_fd(void);
#endif
