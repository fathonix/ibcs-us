/*
 * Copyright (c) 1993  Joe Portman (baron@hebron.connected.com)
 * Copyright (c) 1993, 1994  Drew Sullivan (re-worked for iBCS2)
 * Copyright (c) 2000  Christoph Hellwig (rewrote lookup-related code)
 */
#include <ibcs-us/ibcs/map.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-svr4/statfs.h>
#include <ibcs-us/per-svr4/stat.h>
#include <ibcs-us/per-svr4/sysent.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/dirent.h>
#include <ibcs-us/linux26-compat/linux/err.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/fcntl.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/namei.h>
#include <ibcs-us/linux26-compat/linux/net.h>
#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/signal.h>
#include <ibcs-us/linux26-compat/linux/slab.h>
#include <ibcs-us/linux26-compat/linux/socket.h>
#include <ibcs-us/linux26-compat/linux/stat.h>
#include <ibcs-us/linux26-compat/linux/string.h>
#include <ibcs-us/linux26-compat/linux/time.h>
#include <ibcs-us/linux26-compat/linux/types.h>
#include <ibcs-us/linux26-compat/linux/un.h>
#include <ibcs-us/linux26-compat/linux/utime.h>

#define dirent	linux_dirent64

static int
copy_kstatfs(struct svr4_statfs *buf, struct kstatfs *st)
{
	struct svr4_statfs ibcsstat;

	ibcsstat.f_type = st->f_type;
	ibcsstat.f_bsize = st->f_bsize;
	ibcsstat.f_frsize = 0;
	ibcsstat.f_blocks = st->f_blocks;
	ibcsstat.f_bfree = st->f_bfree;
	ibcsstat.f_files = st->f_files;
	ibcsstat.f_ffree = st->f_ffree;
	memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
	memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

	/* Finally, copy it to the user's buffer */
	return copy_to_user(buf, &ibcsstat, sizeof(struct svr4_statfs));
}

int svr4_statfs(const char * path, struct svr4_statfs * buf, int len, int fstype)
{
	struct svr4_statfs ibcsstat;

	if (len > (int)sizeof(struct svr4_statfs))
		return -EINVAL;

	if (!fstype) {
#if _KSL > 26
		struct path pathstruct;
#else
		struct nameidata nd;
#endif
		int error;

#if _KSL > 26
		error = user_path(path, &pathstruct);
#else
		error = user_path_walk(path, &nd);
#endif

		if (!error) {
			struct kstatfs tmp;

#if _KSL < 18
			error = vfs_statfs(nd.dentry->d_inode->i_sb, &tmp);
#else
#if _KSL > 24
#if _KSL_IBCS_US
			error = vfs_statfs(pathstruct.mnt->mnt_sb->s_root, &tmp);
#else
#if _KSL > 26
			error = vfs_statfs(pathstruct.dentry->d_inode->i_sb->s_root, &tmp);
#else
			error = vfs_statfs(nd.path.dentry->d_inode->i_sb->s_root, &tmp);
#endif
#endif
#else
			error = vfs_statfs(nd.dentry->d_inode->i_sb->s_root, &tmp);
#endif
#endif
			if (!error && copy_kstatfs(buf, &tmp))
				error = -EFAULT;
#if _KSL < 25
			path_release(&nd);
#else
#if _KSL > 26
			path_put(&pathstruct);
#else
			path_put(&nd.path);
#endif
#endif
		}

		return error;
	}

	/*
	 * Linux can't stat unmounted filesystems so we
	 * simply lie and claim 500MB of 8GB is free. Sorry.
	 */
	ibcsstat.f_bsize = 1024;
	ibcsstat.f_frsize = 0;
	ibcsstat.f_blocks = 8 * 1024 * 1024;	/* 8GB */
	ibcsstat.f_bfree = 500 * 1024;		/* 100MB */
	ibcsstat.f_files = 60000;
	ibcsstat.f_ffree = 50000;
	memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
	memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

	/* Finally, copy it to the user's buffer */
	return copy_to_user(buf, &ibcsstat, len) ? -EFAULT : 0;
}

int svr4_fstatfs(unsigned int fd, struct svr4_statfs * buf, int len, int fstype)
{
	struct svr4_statfs ibcsstat;

	if (len > (int)sizeof(struct svr4_statfs))
		return -EINVAL;

	if (!fstype) {
		struct file * file;
		struct kstatfs tmp;
		int error;

		error = -EBADF;
		file = fget(fd);
		if (!file)
			goto out;
#if _KSL < 18
		error = vfs_statfs(file->f_dentry->d_inode->i_sb, &tmp);
#else
#if _KSL_IBCS_US
		struct path linux26_path;
		error = vfs_statfs(inode_get_i_sb(file->f_dentry->d_inode, &linux26_path)->s_root, &tmp);
#else
		error = vfs_statfs(file->f_dentry->d_inode->i_sb->s_root, &tmp);
#endif
#endif
		if (!error && copy_kstatfs(buf, &tmp))
			error = -EFAULT;
		fput(file);

out:
		return error;
	}

	/*
	 * Linux can't stat unmounted filesystems so we
	 * simply lie and claim 500MB of 8GB is free. Sorry.
	 */
	ibcsstat.f_bsize = 1024;
	ibcsstat.f_frsize = 0;
	ibcsstat.f_blocks = 8 * 1024 * 1024;	/* 8GB */
	ibcsstat.f_bfree = 500 * 1024;		/* 100MB */
	ibcsstat.f_files = 60000;
	ibcsstat.f_ffree = 50000;
	memset(ibcsstat.f_fname, 0, sizeof(ibcsstat.f_fname));
	memset(ibcsstat.f_fpack, 0, sizeof(ibcsstat.f_fpack));

	/* Finally, copy it to the user's buffer */
	return copy_to_user(buf, &ibcsstat, len) ? -EFAULT : 0;
}

int svr4_open(const char *fname, int flag, int mode)
{
	int error, fd;
#ifdef __sparc__
	err=SYS(open,fname, map_flags(flag, fl_svr4_to_linux), mode);return err;
#else
	u_long args[3];
	struct file *file;
	mm_segment_t old_fs;
	const char *p;
	struct sockaddr_un addr;

	fd = SYS(open,fname, map_flags(flag, fl_svr4_to_linux), mode);
	if (fd < 0)
		return fd;

	/* Sometimes a program may open a pathname which it expects
	 * to be a named pipe (or STREAMS named pipe) when the
	 * Linux domain equivalent is a Unix domain socket. (e.g.
	 * UnixWare uses a STREAMS named pipe /dev/X/Nserver.0 for
	 * X :0 but Linux uses a Unix domain socket /tmp/.X11-unix/X0)
	 * It isn't enough just to make the symlink because you cannot
	 * open() a socket and read/write it. If we spot the error we can
	 * switch to socket(), connect() and things will likely work
	 * as expected however.
	 */
	file = fget(fd);
	if (!file)
		return fd; /* Huh?!? */
	if (!S_ISSOCK(file->f_dentry->d_inode->i_mode)) {
		fput(file);
		return fd;
	}
	fput(file);

	SYS(close,fd);
	args[0] = AF_UNIX;
	args[1] = SOCK_STREAM;
	args[2] = 0;
	old_fs = get_fs();
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	fd = SYS(socket, args[0], args[1], args[2]);
#else
	fd = SYS(socketcall,SYS_SOCKET, args);
#endif
	set_fs(old_fs);
	if (fd < 0)
		return fd;

	p = getname(fname);
	if (IS_ERR(p)) {
		SYS(close,fd);
		return PTR_ERR(p);
	}
	if (strlen(p) >= UNIX_PATH_MAX) {
		putname(p);
		SYS(close,fd);
		return -E2BIG;
	}
	addr.sun_family = AF_UNIX;
	strcpy(addr.sun_path, p);
	putname(p);

	args[0] = fd;
	args[1] = (int)((long)(&addr));
	args[2] = sizeof(struct sockaddr_un);
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	error = SYS(connect, args[0], args[1], args[2]);
#else
	error = SYS(socketcall,SYS_CONNECT, args);
#endif
	set_fs(old_fs);
	if (error) {
		SYS(close,fd);
		return error;
	}

	return fd;
#endif
}

#define NAME_OFFSET(de)	((int) ((de)->d_name - (char *) (de)))
#define ROUND_UP(x)	(((x)+sizeof(long)-1) & ~(sizeof(long)-1))

struct svr4_getdents_callback {
	struct dirent * current_dir;
	struct dirent * previous;
	int count;
	int error;
};

static int svr4_filldir(void * __buf, const char * name, int namlen,
	loff_t offset, u64 ino, unsigned int d_type)
{
	struct dirent * dirent;
	struct svr4_getdents_callback * buf = (struct svr4_getdents_callback *) __buf;
	int reclen = ROUND_UP(NAME_OFFSET(dirent) + namlen + 1);

	buf->error = -EINVAL;   /* only used if we fail.. */
	if (reclen > buf->count)
		return -EINVAL;

	dirent = buf->previous;
	if (dirent)
		put_user(offset, &dirent->d_off);
	dirent = buf->current_dir;
	buf->previous = dirent;

#ifdef CONFIG_ABI_SHINOMAP
	ino = linux_to_svr4_ino_t(abi_map(ino,1));
#else
	ino = abi_map(ino,1);

	if (current->personality & SHORT_INODE) {
		/* read() on a directory only handles
		 * short inodes but cannot use 0 as that
		 * indicates an empty directory slot.
		 * Therefore stat() must also fold
		 * inode numbers avoiding 0. Which in
		 * turn means that getdents() must fold
		 * inodes avoiding 0 - if the program
		 * was built in a short inode environment.
		 * If we have short inodes in the dirent
		 * we also have a two byte pad so we
		 * can let the high word fall in the pad.
		 * This makes it a little more robust if
		 * we guessed the inode size wrong.
		 */
		if (!((unsigned long)ino & 0xffff)) ino = 0xfffffffe;
	}
#endif

	put_user(ino, &dirent->d_ino);
	put_user(reclen, &dirent->d_reclen);
	if (copy_to_user(dirent->d_name, name, namlen))
		return -EFAULT;
	char* name_end = &dirent->d_name[namlen];
	put_user(0, name_end);
	{	char *ptr = (char *)dirent;
		ptr += reclen;
		dirent = (struct dirent *)ptr;
	}
	buf->current_dir = dirent;
	buf->count -= reclen;
	return 0;
}



int svr4_getdents(int fd, char *dirent, int count)
{
	struct file * file;
	struct dirent * lastdirent;
	struct svr4_getdents_callback buf;
	int error;

	error = -EBADF;
	file = fget(fd);
	if (!file)
		goto out;

	buf.current_dir = (struct dirent *) dirent;
	buf.previous = NULL;
	buf.count = count;
	buf.error = 0;
	error = vfs_readdir(file, (filldir_t)svr4_filldir, &buf);
	if (error < 0)
		goto out_putf;
	error = buf.error;
	lastdirent = buf.previous;
	if (lastdirent) {
#if _KSL_IBCS_US
		put_user(file_get_f_pos(file), &lastdirent->d_off);
#else
		put_user(file->f_pos, &lastdirent->d_off);
#endif
		error = count - buf.count;
	}

out_putf:
	fput(file);

out:
	return error;
}

EXPORT_SYMBOL(svr4_fstatfs);
EXPORT_SYMBOL(svr4_getdents);
EXPORT_SYMBOL(svr4_open);
EXPORT_SYMBOL(svr4_statfs);
