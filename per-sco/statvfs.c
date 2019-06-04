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
 * SCO OpenServer statvfs/fstatvfs support.
 */
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-sco/types.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/err.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/mount.h>
#include <ibcs-us/linux26-compat/linux/namei.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/statfs.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/string.h>


static int
report_statvfs(struct vfsmount *mnt, struct inode *ip, struct sco_statvfs *bufp)
{
	struct sco_statvfs buf;
	struct kstatfs s;
	int error;
	struct super_block* sb;

#if _KSL < 18
	error = vfs_statfs(mnt->mnt_sb, &s);
#else
	error = vfs_statfs(mnt->mnt_sb->s_root, &s);
#endif
	if (error)
		return error;

	memset(&buf, 0, sizeof(struct sco_statvfs));

	buf.f_bsize	= s.f_bsize;
	buf.f_frsize	= s.f_bsize;
	buf.f_blocks	= s.f_blocks;
	buf.f_bfree	= s.f_bfree;
	buf.f_bavail	= s.f_bavail;
	buf.f_files	= s.f_files;
	buf.f_free	= s.f_ffree;
	buf.f_favail	= s.f_ffree; /* SCO addition in the middle! */
#if	_KSL_IBCS_US
	struct path	path;
	sb		= inode_get_i_sb(ip, &path);
#else
	sb		= ip->i_sb;
#endif
	buf.f_sid	= sb->s_dev;

	/*
	 * Get the name of the filesystem.
	 *
	 * Sadly, some code "in the wild" actually checks the name
	 * against a hard coded list to see if it is a "real" fs or not.
	 *
	 * I believe Informix Dynamic Server for SCO is one such.
	 */
	if (strncmp(sb->s_type->name, "ext2", 4) == 0)
		strcpy(buf.f_basetype, "HTFS");
	else
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

	if (copy_to_user(bufp, &buf, sizeof(struct sco_statvfs)))
		return -EFAULT;
	return 0;
}

int
sco_statvfs(char *filename, struct sco_statvfs *bufp)
{
#if _KSL > 26
	struct path path;
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
	}
	return error;
}

int
sco_fstatvfs(int fd, struct sco_statvfs *bufp)
{
#if	_KSL_IBCS_US
    	struct path	path;
	int		error;

	error = linux26_user_fpath(fd, &path);
	if (error < 0) {
		error = report_statvfs(path.mnt, path.dentry->d_inode, bufp);
	}
#else
	struct file *fp;
	int error = -EBADF;

	fp = fget(fd);
	if (fp) {
		error = report_statvfs(fp->f_vfsmnt,
				fp->f_dentry->d_inode, bufp);
		fput(fp);
	}
#endif
	return error;
}
