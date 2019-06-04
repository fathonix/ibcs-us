#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/types.h>


int
svr4_console_ioctl(int fd, u_int cmd, caddr_t data)
{
	switch (cmd) {
	case 0x6301: /* CONS_CURRENT: Get display adapter type */
	case 0x6302: /* CONS_GET: Get display mode setting */
		/*
		 * Always error so the application never tries
		 * anything overly fancy on the console.
		 */
		return -EINVAL;
	case 0x4304: /* _TTYDEVTYPE */
		/* If on console then 1, if pseudo tty then 2 */
		return 2;
	}

	printk(KERN_ERR "iBCS: console ioctl %d unsupported\n", cmd);
	return -EINVAL;
}

int
svr4_video_ioctl(int fd, u_int cmd, caddr_t data)
{
	switch (cmd) {
	case 1: /* MAP_CLASS */
		/* Get video memory map & IO privilege */
                break;

	/* This doesn't agree with my SCO 3.2.4 ???? */
	case 4: /* C_IOC */
		/* see /etc/conf/pack.d/cn/class.h on any SCO unix box :-) */
                break;

        default:
                break;
	}

	printk(KERN_ERR "iBCS: video ioctl %d unsupported\n", cmd);
	return -EINVAL;
}

EXPORT_SYMBOL(svr4_console_ioctl);
EXPORT_SYMBOL(svr4_video_ioctl);
