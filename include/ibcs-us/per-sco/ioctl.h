#ifndef _IBCS_US_SCO_IOCTL_H
#define _IBCS_US_SCO_IOCTL_H

#include <ibcs-us/linux26-compat/linux/types.h>

/* tapeio.c */
extern int	sco_tape_ioctl(int, u_int, caddr_t);

/* termios.c */
extern int	sco_term_ioctl(int, u_int, caddr_t);

/* From vtkd.c */
extern int	sco_vtkbd_ioctl(int, u_int, caddr_t);

#endif /* _IBCS_US_SCO_IOCTL_H */
