#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-svr4/ioctl.h>
#include <ibcs-us/per-svr4/socksys.h> 		/* for socksys_fdinit */
#include <ibcs-us/per-svr4/stream.h>
#include <ibcs-us/per-svr4/tli.h>

#include <ibcs-us/linux26-compat/asm/ioctls.h>
#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/err.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kdev_t.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/slab.h>
#include <ibcs-us/linux26-compat/linux/sockios.h>


/*
 * Check if the inode belongs to /dev/socksys.
 */
#define IS_SOCKSYS(ip) (MAJOR((ip)->i_rdev) == SOCKSYS_MAJOR)


static int
i_nread(u_int fd, struct file *fp, struct inode *ip,
		void *data, struct pt_regs *regs)
{
	int error=0;

	if (!access_ok(VERIFY_WRITE, data, sizeof(u_long))) {
		error = -EFAULT;
		goto fput;
	}

#if defined(CONFIG_ABI_XTI)
	if (S_ISSOCK(ip->i_mode)) {
		struct T_private *ti = Priv(fp);

		if (IS_SOCKSYS(ip))
			timod_update_socket(fd, fp, regs);

		if (ti && ti->pfirst) {
			put_user(ti->pfirst->length, (u_long *)data);
			fput(fp);
			return 1; /* at least 1... (FIXME) */
		}
	}

	fput(fp);
#endif

	error = SYS(ioctl,fd, TIOCINQ, (long)data);
	if (error == -EINVAL)
		return 0;
	else if (error)
		return error;

	__get_user(error, (u_long *)data);
	return !!error;
fput:
	fput(fp);
	return error;
}

static int
i_peek(u_int fd, struct file *fp, struct inode *ip,
		void *data, struct pt_regs *regs)
{
#if defined(CONFIG_ABI_XTI)
	fput(fp);
	return 0;
#else
	struct T_private	*ti = Priv(fp);
	struct T_primsg		*tp;
	struct strpeek		buf, *uap = data;
	int			error = -EFAULT;

	if (copy_from_user(&buf, uap, sizeof(buf)))
		goto fput;

	error = 0;
	if (!S_ISSOCK(ip->i_mode))
		goto fput;

	if (IS_SOCKSYS(ip))
		timod_update_socket(fd, fp, regs);

	if (!ti || !ti->pfirst)
		goto fput;
	tp = ti->pfirst;

	error = -EFAULT;
	if (!buf.flags || buf.flags == tp->pri) {
		int	l;



		if (buf.ctl.maxlen <= tp->length)
			l = buf.ctl.maxlen;
		else
			l = tp->length;

		if (copy_to_user(buf.ctl.buf,
		    ((char *)&tp->type) + ti->offset, l))
			goto fput;

		if (put_user(l, &uap->ctl.len))
			goto fput;

		if (buf.dat.maxlen >= 0 && put_user(0, &uap->dat.len))
			goto fput;

		if (put_user(tp->pri, &uap->flags))
			goto fput;

		error = 1;
	}
fput:
	fput(fp);
	return error;
#endif /* CONFIG_ABI_XTI */
}

static int
i_str(u_int fd, struct file *fp, struct inode *ip,
		void *data, struct pt_regs *regs)
{
	/*
	 * Unpack the ioctl data and forward as a normal
	 * ioctl. Timeouts are not handled (yet?).
	 */
	struct strioctl {
		int cmd, timeout, len;
		char *data;
	} it, *uap = data;

	if (copy_from_user(&it, uap, sizeof(struct strioctl)))
		return -EFAULT;

#if defined(CONFIG_ABI_TRACE)
	abi_trace(ABI_TRACE_STREAMS, "STREAMS I_STR ioctl(%d, 0x%08x, %p)\n",
			fd, it.cmd, it.data);
#endif

#ifdef CONFIG_ABI_XTI
	if ((it.cmd >> 8) == 'T')
		return timod_ioctl(regs, fd, it.cmd & 0xff, it.data, it.len,
			&uap->len);
#endif
	return __svr4_ioctl(regs, fd, it.cmd, it.data);
}

int
svr4_stream_ioctl(struct pt_regs *regs, int fd, u_int cmd, caddr_t data)
{
	struct file		*fp;
	struct inode		*ip;
	int			error;

	fp = fget(fd);
	if (!fp)
		return -EBADF;
	ip = fp->f_dentry->d_inode;

	/*
	 * Special hack^H^Hndling for socksys fds
	 */
	if (S_ISSOCK(ip->i_mode) == 0 && IS_SOCKSYS(ip)) {
		error = socksys_fdinit(fd, 0, NULL, NULL);
		if (error < 0)
			return error;
		fput(fp);
		fp = fget(fd);
		if (!fp)
			return -EBADF;
		ip = fp->f_dentry->d_inode;
	}

	switch (cmd) {
	case 001: /* I_NREAD */
		return i_nread(fd, fp, ip, data, regs);

	case 017: /* I_PEEK */
		return i_peek(fd, fp, ip, data, regs);
	}

	fput(fp);

	switch (cmd) {
	case 010: /* I_STR */
		return i_str(fd, fp, ip, data, regs);
	case 002: { /* I_PUSH */
		const char *tmp;

		/* Get the name anyway to validate it. */
		tmp = getname(data);
		if (IS_ERR(tmp))
			return PTR_ERR(tmp);

#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_STREAMS,
				"%d STREAMS I_PUSH %s\n", fd, tmp);
#endif

		putname(tmp);
		return 0;
	}
	case 003: /* I_POP */
#if defined(CONFIG_ABI_TRACE)
		  abi_trace(ABI_TRACE_STREAMS, "%d STREAMS I_POP\n", fd);
#endif
		  return 0;

	case 005: /* I_FLUSH */
		  return 0;

	case 013: { /* I_FIND */
		const char *tmp;

		/* Get the name anyway to validate it. */
		tmp = getname(data);
		if (IS_ERR(tmp))
				return PTR_ERR(tmp);

#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_STREAMS,
				"%d STREAMS I_FIND %s\n", fd, tmp);
#endif
#ifdef CONFIG_ABI_XTI
		if (!strcmp(tmp, "timod")) {
			putname(tmp);
			return 1;
		}
#endif
		putname(tmp);
		return 0;
	}

	/* FIXME: These are bogus. */
	case 011: /* I_SETSIG */
		error=SYS(ioctl,fd, FIOSETOWN, (long)current->pid); return error;
	case 012: /* I_GETSIG */
		error=SYS(ioctl,fd, FIOGETOWN, (long)data); return error;

	case 020: /* I_FDINSERT */
#ifdef CONFIG_ABI_XTI
		return stream_fdinsert(regs, fd,
				(struct strfdinsert *)data);
#else
		return -EINVAL;
#endif

	case 004: /* I_LOOK */
	case 006: /* I_SRDOPT */
	case 007: /* I_GRDOPT */
	case 014: /* I_LINK */
	case 015: /* I_UNLINK */
	case 021: /* I_SENDFD */
	case 022: /* I_RECVFD */
	case 023: /* I_SWROPT */
	case 040: /* I_SETCLTIME */
		return 0; /* Lie... */
	case 042: /* I_CANPUT */
		/*
		 * Arg is the priority band in question. We only
		 * support one priority band so data must be 0.
		 * If the band is writable we should return 1, if
		 * the band is flow controlled we should return 0.
		 */
		if (data)
			return -EINVAL;

		/* FIXME: How can we test if a write would block? */
		return 1;

	case 024: /* I_GWROPT */
	case 025: /* I_LIST */
	case 026: /* I_PLINK */
	case 027: /* I_PUNLINK */
	case 030: /* I_SETEV */
	case 031: /* I_GETEV */
	case 032: /* I_STREV */
	case 033: /* I_UNSTREV */
	case 034: /* I_FLUSHBAND */
	case 035: /* I_CKBAND */
	case 036: /* I_GETBAND */
	case 037: /* I_ATMARK */
	case 041: /* I_GETCLTIME */
			/* Unsupported - drop out. */
                break;

        default:
                break;
	}

	printk(KERN_ERR "iBCS: STREAMS ioctl 0%o unsupported\n", cmd);
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_stream_ioctl);
