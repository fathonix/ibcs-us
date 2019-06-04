#ifndef _IBCS_US_SVR4_SIGACTION_H
#define _IBCS_US_SVR4_SIGACTION_H

#include <ibcs-us/linux26-compat/linux/signal.h>

//#ident "%W% %G%"

/* signal.c */
struct task_struct;
extern void deactivate_signal(struct task_struct *, int);

struct svr4_sigaction {
       int          svr4_sa_flags;
       __sighandler_t svr4_sa_handler;
       unsigned long svr4_sa_mask;
       int	    svr4_sa_resv[2];  /* Reserved for something or another */
};
#define SVR4_SA_ONSTACK   1
#define SVR4_SA_RESETHAND 2
#define SVR4_SA_RESTART   4
#define SVR4_SA_SIGINFO   8
#define SVR4_SA_NODEFER  16
#define SVR4_SA_NOCLDWAIT 0x10000
#define SVR4_SA_NOCLDSTOP 0x20000

#endif /* _IBCS_US_SVR4_SIGACTION_H */
