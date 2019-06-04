
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/per-svr4/sigset.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/socket.h>
#include <ibcs-us/linux26-compat/linux/types.h>


int sol_llseek(struct pt_regs * regs)
{
	unsigned int fd;
	unsigned long offset_high, offset_low;
	unsigned origin;
	long long res;
	unsigned int rvalue;
	mm_segment_t old_fs;
	struct inode *inode;
	struct file *file;
	get_user(fd, ((unsigned int *)_SP(regs))+1);
	get_user(offset_low, ((unsigned int *)_SP(regs))+2);
	get_user(offset_high, ((unsigned int *)_SP(regs))+3);
	get_user(origin, ((unsigned int *)_SP(regs))+4);

	old_fs = get_fs();
	set_fs(get_ds());
#ifdef CONFIG_65BIT
	rvalue = SYS(lseek,fd,(offset_high << 32) + offset_low,origin);
	res = rvalue;
#else
	rvalue = SYS(_llseek,fd,offset_high,offset_low,&res,origin);
#endif
	set_fs(old_fs);

	if ( rvalue < -ENOIOCTLCMD) {
		_DX(regs) = (res >> 32);
		rvalue = (res & 0xffffffff);
	}
	else if (rvalue == -ESPIPE) {
		/* Solaris allows you to seek on a pipe */
		file = fget(fd);
		if (file) {
			inode = file->f_dentry->d_inode;
			if (inode && (S_ISCHR(inode->i_mode)
				      || S_ISBLK(inode->i_mode))) {
				rvalue = 0;
				_DX(regs) = 0;
			}
			fput(file);
		}
	}

	return rvalue;
}

int sol_memcntl(unsigned addr, unsigned len, int cmd, unsigned arg,
		 int attr, int mask)
{
	return 0;
}


enum {
	GETACL = 1,
	SETACL = 2,
	GETACLCNT = 3
};

int sol_acl(char *pathp, int cmd, int nentries, void *aclbufp)
{
	switch (cmd) {
	case GETACLCNT:
		return 0;

	case GETACL:
		return -EIO;

	case SETACL:
		return -EPERM;

	default:
		return -EINVAL;
	}
}
