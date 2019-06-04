/*
 * Copyright 1995, 1996  Mike Jagdis (jaggy@purplet.demon.co.uk)
 */
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-svr4/ioctl.h>
#include <ibcs-us/per-svr4/stream.h>
#include <ibcs-us/per-svr4/tli.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/fcntl.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/in.h>
#include <ibcs-us/linux26-compat/linux/kdev_t.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/poll.h>
#include <ibcs-us/linux26-compat/linux/ptrace.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/slab.h>
#include <ibcs-us/linux26-compat/linux/socket.h>
#include <ibcs-us/linux26-compat/linux/types.h>
#include <ibcs-us/linux26-compat/linux/un.h>
#include <ibcs-us/linux26-compat/net/sock.h>


/*
 * Check if the inode belongs to /dev/socksys.
 */
#define IS_SOCKSYS(ip) (MAJOR((ip)->i_rdev) == SOCKSYS_MAJOR)


int
timod_getmsg(int fd, struct inode *ip, int pmsg, struct pt_regs *regs)
{
	struct strbuf	ctl, *ctlp, dat, *datp;
	int		error;

	ctlp = (struct strbuf *)get_syscall_parameter(regs, 1);
	datp = (struct strbuf *)get_syscall_parameter(regs, 2);

	if (ctlp) {
		if (copy_from_user(&ctl, ctlp, sizeof(ctl)))
			return -EFAULT;
		if ((error = put_user(-1, &ctlp->len)))
			return error;
	} else
		ctl.maxlen = -1;

	if (datp) {
		if (copy_from_user(&dat, datp, sizeof(dat)))
			return -EFAULT;
		if ((error = put_user(-1, &datp->len)))
			return error;
	} else
		dat.maxlen = -1;

#ifdef CONFIG_ABI_SPX
	if (IS_SOCKSYS(ip) && MINOR(ip->i_rdev) == 1) {

#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_STREAMS,
			"SPX: getmsg offers descriptor %d\n", fd);
#endif

		if ((error = put_user(fd, ctl.buf)))
			return error;
		if ((error = put_user(4, &ctlp->len)))
			return error;

		return 0;
	}
#endif /* CONFIG_ABI_SPX */

#ifdef CONFIG_ABI_XTI
	int		flags;
	int*		flagsp;

	if (pmsg) {
		/* int* bandp = (int *)get_syscall_parameter(regs, 3); */
		flagsp = (int *)get_syscall_parameter(regs, 4);
	} else
		flagsp = (int *)get_syscall_parameter (regs, 3);

	if ((error = get_user(flags, flagsp)))
		return error;

	if (flags == 0 || flags == MSG_HIPRI ||
	    flags == MSG_ANY || flags == MSG_BAND) {
		struct file	*fp;

		fp = fget(fd);
		error = do_getmsg(fd, regs, ctl.buf, ctl.maxlen, &ctlp->len,
				dat.buf, dat.maxlen, &datp->len, &flags);
		fput(fp);

		if (error >= 0)
			error = put_user(flags, flagsp);
		return error;
	}

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_STREAMS,
			"XTI: getmsg flags value bad (%d) for %d\n",
			flags, fd);
#endif /* CONFIG_ABI_TRACE */
#endif /* CONFIG_ABI_XTI */
	return -EINVAL;
}


int
timod_putmsg(int fd, struct inode *ip, int pmsg, struct pt_regs *regs)
{
	struct strbuf		ctl, *ctlp, dat, *datp;
	int			flags;
#ifdef CONFIG_ABI_SPX
	int			error, newfd;
#endif

	ctlp = (struct strbuf *)get_syscall_parameter(regs, 1);
	datp = (struct strbuf *)get_syscall_parameter(regs, 2);
	if (pmsg) {
		/* int band = get_syscall_parameter(regs, 3); */
		flags = get_syscall_parameter(regs, 4);
	} else
		flags = get_syscall_parameter(regs, 3);

	if (ctlp) {
		if (copy_from_user(&ctl, ctlp, sizeof(ctl)))
			return -EFAULT;
		if (ctl.len < 0 && flags)
			return -EINVAL;
	} else {
		ctl.len = 0;
		ctl.buf = NULL;
	}

	if (datp) {
		if (copy_from_user(&dat, datp, sizeof(dat)))
			return -EFAULT;
	} else {
		dat.len = 0;
		dat.buf = NULL;
	}

#ifdef CONFIG_ABI_SPX
	if (IS_SOCKSYS(ip) && MINOR(ip->i_rdev) == 1) {
		if (ctl.len != 4)
			return -EIO;

		error = get_user(newfd, ctl.buf);
		if (error)
			return error;

#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_STREAMS,
				"SPX: putmsg on %d dups descriptor %d\n",
				fd, newfd);
#endif
		error = SYS(dup2,newfd, fd);

		return (error < 0 ? error : 0);
	}
#endif /* CONFIG_ABI_SPX */

#ifdef CONFIG_ABI_XTI
	return do_putmsg(fd, regs, ctl.buf, ctl.len,
			dat.buf, dat.len, flags);
#endif
	return -EINVAL;
}

int
stream_fdinsert(struct pt_regs *regs, int fd, struct strfdinsert *arg)
{
	struct strfdinsert	sfd;

	if (copy_from_user(&sfd, arg, sizeof(sfd)))
		return -EFAULT;

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_STREAMS,
			"%u fdinsert: flags=%ld, fildes=%u, offset=%d\n",
			fd, sfd.flags, sfd.fildes, sfd.offset);
#endif
#ifdef CONFIG_ABI_XTI
	return do_putmsg(fd, regs, sfd.ctlbuf.buf, sfd.ctlbuf.len,
			sfd.datbuf.buf, sfd.datbuf.len, sfd.fildes);
#else
	return -EINVAL;
#endif
}
