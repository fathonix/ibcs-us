/*
 *   abi/uw7/misc.c - various UW7 system calls.
 *
 *  This software is under GPL
 */
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/trace.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/file.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/sched.h>


int uw7_sleep(int seconds)
{
	struct timespec t;
	mm_segment_t old_fs;
	int error;

	t.tv_sec = seconds;
	t.tv_nsec = 0;
	old_fs = get_fs();
	set_fs(get_ds());
	error = SYS_NATIVE(nanosleep,&t, NULL);
	set_fs(old_fs);
	return error;
}

#define UW7_MAXUID      60002

int uw7_seteuid(int uid)
{
	int retval;
	if (uid < 0 || uid > UW7_MAXUID)
		return -EINVAL;
	retval = SYS(setreuid,-1, uid); return retval;
}

int uw7_setegid(int gid)
{
	int retval;
	if (gid < 0 || gid > UW7_MAXUID)
		return -EINVAL;
	retval = SYS(setreuid,-1, gid); return retval;
}

/* can't call sys_pread64() directly because off is 32bit on UW7 */
int uw7_pread(unsigned int fd, char * buf, int count, long off)
{ 
	int retval;
	retval=SYS(pread64,fd, buf, count, (loff_t)off); return retval;
}

/* can't call sys_pwrite64() directly because off is 32bit on UW7 */
int uw7_pwrite(unsigned int fd, char * buf, int count, long off)
{
	int retval;
	retval=SYS(pwrite64,fd, buf, count, (loff_t)off); return retval;
}

int uw7_stty(int fd, int cmd)
{
	return -EIO;
}

int uw7_gtty(int fd, int cmd)
{
	return -EIO;
}
