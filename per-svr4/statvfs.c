/*
 * Copyright (c) 2001 Caldera Deutschland GmbH.
 * Copyright (c) 2001 Christoph Hellwig.
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
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
 * SVR4 statvfs/fstatvfs support.
 */
#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/mount.h>
#include <ibcs-us/linux26-compat/linux/namei.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/statfs.h>
#include <ibcs-us/linux26-compat/linux/string.h>


struct svr4_statvfs {
	u_int32_t  f_bsize;
	u_int32_t  f_frsize;
	u_int32_t  f_blocks;
	u_int32_t  f_bfree;
	u_int32_t  f_bavail;
	u_int32_t  f_files;
	u_int32_t  f_free;
	u_int32_t  f_sid;
	char	   f_basetype[16];
	u_int32_t  f_flag;
	u_int32_t  f_namemax;
	char	   f_fstr[32];
	u_int32_t  f_filler[16];
};

static int
report_statvfs(struct vfsmount *mnt, struct inode *ip, struct svr4_statvfs *bufp)
{
	struct svr4_statvfs buf;
	struct kstatfs s;
	int error;

#if _KSL < 18
	error = vfs_statfs(mnt->mnt_sb, &s);
#else
	error = vfs_statfs(mnt->mnt_sb->s_root, &s);
#endif
	if (error)
		return error;

	memset(&buf, 0, sizeof(struct svr4_statvfs));

	buf.f_bsize	= s.f_bsize;
	buf.f_frsize	= s.f_bsize;
	buf.f_blocks	= s.f_blocks;
	buf.f_bfree	= s.f_bfree;
	buf.f_bavail	= s.f_bavail;
	buf.f_files	= s.f_files;
	buf.f_free	= s.f_ffree;
#ifdef	_KSL_IBCS_US
	struct super_block* sb = mnt->mnt_sb;
#else
	struct super_block* sb = ip->i_sb;
#endif
	buf.f_sid	= sb->s_dev;

	/* Get the name of the filesystem */
	strcpy(buf.f_basetype, sb->s_type->name);

	/* Check for a few flags statvfs wants but statfs doesn't have. */
#if	_KSL_IBCS_US
	if (sb->s_flags & MS_RDONLY)
#else
	if (IS_RDONLY(ip))
#endif
		buf.f_flag |= 1;
	if (mnt->mnt_flags & MNT_NOSUID)
		buf.f_flag |= 2;

	buf.f_namemax	= s.f_namelen;

	if (copy_to_user(bufp, &buf, sizeof(struct svr4_statvfs)))
		return -EFAULT;
	return 0;
}

int
svr4_statvfs(char *filename, struct svr4_statvfs *bufp)
{
#if _KSL > 26
	struct path      path;
#else
	struct nameidata nd;
#endif
	int error;

#if _KSL > 26
	error = user_path(filename, &path);
#else
	error = user_path_walk(filename, &nd);
#endif
	if (!error) {
#if _KSL_IBCS_US
		error = report_statvfs(path.mnt, NULL, bufp);
		path_put(&path);
#else
#if _KSL > 24
#if _KSL > 26
		error = report_statvfs(path.mnt, path.dentry->d_inode, bufp);
		path_put(&path);
#else
		error = report_statvfs(nd.path.mnt, nd.path.dentry->d_inode, bufp);
		path_put(&nd.path);
#endif
#else
		error = report_statvfs(nd.mnt, nd.dentry->d_inode, bufp);
		path_release(&nd);
#endif
#endif
	}
	return error;
}

int
svr4_fstatvfs(int fd, struct svr4_statvfs *bufp)
{
	struct file *fp;
	int error = -EBADF;

	fp = fget(fd);
	if (fp) {
#if	_KSL_IBCS_US
		struct path path;
		linux26_user_fpath(fd, &path);
		error = report_statvfs(path.mnt, NULL, bufp);
#else
		error = report_statvfs(fp->f_vfsmnt,
				fp->f_dentry->d_inode, bufp);
#endif
		fput(fp);
	}
	return error;
}

EXPORT_SYMBOL(svr4_statvfs);
EXPORT_SYMBOL(svr4_fstatvfs);
