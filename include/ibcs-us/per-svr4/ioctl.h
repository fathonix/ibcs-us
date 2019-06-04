#ifndef _IBCS_US_SVR4_IOCTL_H
#define _IBCS_US_SVR4_IOCTL_H

#include <ibcs-us/linux26-compat/asm/ptrace.h>
#include <ibcs-us/linux26-compat/linux/types.h>

/*
 * Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 *
 * Note that Linux does the same but has the IOC_IN and IOC_OUT values
 * round the other way and uses 0 for IOC_VOID.
 */
enum {
	/* parameter length, at most 13 bits */
	BSD_IOCPARM_MASK	= 0x1fff,
	/* no parameters */
	BSD_IOC_VOID		= 0x20000000,
	/* copy out parameters */
	BSD_IOC_OUT		= 0x40000000,
	/* copy in parameters */
	BSD_IOC_IN		= 0x80000000,
	/* possibly copy in and out parameters */
	BSD_IOC_INOUT		= BSD_IOC_IN|BSD_IOC_OUT,
};


#define BSD__IOC(inout,group,num,len) \
	(inout | ((len & BSD_IOCPARM_MASK) << 16) | ((group) << 8) | (num))

#define	BSD__IO(g,n)		BSD__IOC(BSD_IOC_VOID, (g), (n), 0)
#define	BSD__IOR(g,n,t)		BSD__IOC(BSD_IOC_OUT, (g), (n), sizeof(t))
#define	BSD__IOW(g,n,t)		BSD__IOC(BSD_IOC_IN, (g), (n), sizeof(t))
#define	BSD__IOWR(g,n,t)	BSD__IOC(BSD_IOC_INOUT,	(g), (n), sizeof(t))

/* Some SYSV systems exhibit "compatible" BSD ioctls without the bumf. */
#define BSD__IOV(c,d)	(((c) << 8) | (d))


/*
 *  Function prototypes used for SVR4 ioctl emulation.
 */

//#ident "%W% %G%"

extern int      __svr4_ioctl(struct pt_regs *, int, unsigned long, void *);

/* consio.c */
extern int	svr4_console_ioctl(int, u_int, caddr_t);
extern int	svr4_video_ioctl(int, u_int, caddr_t);

/* filio.c */
extern int	svr4_fil_ioctl(int, u_int, caddr_t);

/* sockio.c */
extern int	svr4_stream_ioctl(struct pt_regs *regs, int, u_int, caddr_t);

/* socksys.c */
extern int	abi_ioctl_socksys(int, u_int, caddr_t);

/* tapeio.c */
extern int	svr4_tape_ioctl(int, u_int, caddr_t);

/* termios.c */
extern int	bsd_ioctl_termios(int, u_int, void *);
extern int	svr4_term_ioctl(int, u_int, caddr_t);
extern int	svr4_termiox_ioctl(int, u_int, caddr_t);

/* timod.c */
extern int	svr4_sockmod_ioctl(int, u_int, caddr_t);
extern int      do_getmsg(int, struct pt_regs *, char *, int,
                        int *, char *, int, int *, int *);
extern int      do_putmsg(int, struct pt_regs *, char *, int,
                        char *, int, int);

#endif /* _IBCS_US_SVR4_IOCTL_H */
