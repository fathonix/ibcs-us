/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/fdtable.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_FDTABLE_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_FDTABLE_H
#include <stddef.h>
#include <sys/cdefs.h>

struct fdtable
{
    unsigned int	max_fds;
    struct file**	fd;      /* current fd array */
};

/*
 * Open file table structure
 */
struct files_struct
{
    struct fdtable*	fdt;
    struct fdtable	fdtab;
};

static inline struct fdtable* files_fdtable(struct files_struct* files)
{
    return files->fdt;
}


static inline struct file* fcheck_files(struct files_struct* files, unsigned int fd)
{
    struct fdtable* fdt = files_fdtable(files);
    return fd >= fdt->max_fds ? NULL : fdt->fd[fd];
}

#define fcheck(fd)	fcheck_files(current->files, fd)

#endif
