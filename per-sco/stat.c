/*
 * Copyright (c) 2001 Caldera Deutschland GmbH.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * aint32_t with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * SCO OpenServer xstat/fxstat/lxstat support.
 */
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-sco/stat.h>
#include <ibcs-us/per-sco/types.h>
#include <ibcs-us/per-svr4/stat.h>
#include <ibcs-us/per-svr4/y2k.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/string.h>


enum {SVR4_stat = 1, SCO_xstat = 51};

static int
report_sco_xstat(struct kstat *stp, struct sco_xstat *bufp)
{
	struct sco_xstat buf;

	memset(&buf, 0, sizeof(struct sco_xstat));

	buf.st_dev	= linux_to_sco_dev_t(abi_map(stp->dev,2));
	buf.st_ino	= linux_to_sco_ino_t(abi_map(stp->ino,1), stp->dev);
	buf.st_mode	= stp->mode;
	buf.st_nlink	= stp->nlink;
	buf.st_uid	= linux_to_sco_uid_t(stp->uid);
	buf.st_gid	= linux_to_sco_gid_t(stp->gid);
	buf.st_rdev	= linux_to_sco_dev_t(abi_map(stp->rdev,2));

	if (stp->size > MAX_NON_LFS)
		return -EOVERFLOW;	/* XXX: what to return for SCO?? */

	buf.st_size	= stp->size;

	buf.st_atime	= svr4_y2k_send(stp->atime.tv_sec);
	buf.st_mtime	= svr4_y2k_send(stp->mtime.tv_sec);
	buf.st_ctime 	= svr4_y2k_send(stp->ctime.tv_sec);

	buf.st_blksize	= stp->blksize;
	buf.st_blocks	= stp->blocks;

	strcpy(buf.st_fstype, "ext2");

	buf.st_sco_flags = 0; /* 1 if remote */

	if (copy_to_user(bufp, &buf, sizeof(struct sco_xstat)))
		return -EFAULT;
	return 0;
}

int
sco_xstat(int vers, char *filename, void *bufp)
{
	struct kstat st;
	int error;

	error = vfs_stat(filename, &st);
	if (error)
		return error;

	switch (vers) {
	case SVR4_stat:
		return report_svr4_stat(&st, bufp);
	case SCO_xstat:
		return report_sco_xstat(&st, bufp);
	}

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_API, "xstat version %d not supported\n", vers);
#endif
	return -EINVAL;
}

int
sco_lxstat(int vers, char *filename, void *bufp)
{
	struct kstat st;
	int error;

	error = vfs_lstat(filename, &st);
	if (error)
		return error;

	switch (vers) {
	case SVR4_stat:
		return report_svr4_stat(&st, bufp);
	case SCO_xstat:
		return report_sco_xstat(&st, bufp);
	}

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_API, "lxstat version %d not supported\n", vers);
#endif
	return -EINVAL;
}

int
sco_fxstat(int vers, int fd, void *bufp)
{
	struct kstat st;
	int error;

	error = vfs_fstat(fd, &st);
	if (error)
		return error;

	switch (vers) {
	case SVR4_stat:
		return report_svr4_stat(&st, bufp);
	case SCO_xstat:
		return report_sco_xstat(&st, bufp);
	}

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_API, "fxstat version %d not supported\n", vers);
#endif
	return -EINVAL;
}
