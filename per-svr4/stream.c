/*
 * Copyright 1994,1995	Mike Jagdis (jaggy@purplet.demon.co.uk)
 */
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-svr4/tli.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kdev_t.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/ptrace.h>
#include <ibcs-us/linux26-compat/linux/sched.h>



#if !defined(CONFIG_ABI_XTI) && !defined(CONFIG_ABI_SPX)
#  define timod_getmsg(fd, ino, is_pmsg, regs)	0
#  define timod_putmsg(fd, ino, is_pmsg, regs)	0
#endif

/*
 * Check if the inode belongs to /dev/spx.
 */
#define IS_SPX(ip) ((ip)->i_rdev == MKDEV(SOCKSYS_MAJOR, 1))


int
svr4_getmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (S_ISSOCK(ip->i_mode))
			error = timod_getmsg(fd, ip, 0, regs);
		fput(fp);
	}
	return error;
}

int
svr4_putmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (S_ISSOCK(ip->i_mode) || IS_SPX(ip))
			error = timod_putmsg(fd, ip, 0, regs);
		fput(fp);
	}
	return error;
}

#ifdef CONFIG_ABI_XTI
int
svr4_getpmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (S_ISSOCK(ip->i_mode))
			error = timod_getmsg(fd, ip, 1, regs);
		fput(fp);
	}
	return error;
}

int
svr4_putpmsg(struct pt_regs *regs)
{
	struct file		*fp;
	struct inode		*ip;
	int			fd;
	int			error = -EBADF;

	fd = (int)get_syscall_parameter(regs, 0);
	fp = fget(fd);
	if (fp) {
		ip = fp->f_dentry->d_inode;
		if (S_ISSOCK(ip->i_mode) || IS_SPX(ip))
			error = timod_putmsg(fd, ip, 1, regs);
		fput(fp);
	}
	return error;
}
#endif /* CONFIG_ABI_XTI */

EXPORT_SYMBOL(svr4_getmsg);

#ifdef CONFIG_ABI_XTI
EXPORT_SYMBOL(svr4_getpmsg);
EXPORT_SYMBOL(svr4_putpmsg);
#endif /* CONFIG_ABI_XTI */

EXPORT_SYMBOL(svr4_putmsg);
