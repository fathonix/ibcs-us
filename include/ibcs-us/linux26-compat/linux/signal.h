/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/signal.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_SIGNAL_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_SIGNAL_H
#include <stddef.h>

/*
 * Do NOT use linux/signal.h here.  It gets a "struct sigaction" that
 * compiles cleanly but doesn't work with rt_sigaction().
 */
#define SA_RESTORER	0x04000000

#include <asm-generic/signal.h>
#include <asm-generic/siginfo.h>
#include <ibcs-us/linux26-compat/linux/string.h>
#include <ibcs-us/linux26-compat/linux/types.h>

typedef unsigned long	old_sigset_t;

#define	SIG_OP_SET(set, sig, _OP_) ({					\
    const unsigned g = (sig) - 1;					\
    char* s = (char*)(set);						\
    s[g >> 3] _OP_ (1 << (g & 0x7));					\
})

static inline void sigaddset(sigset_t *set, int sig)
{
    SIG_OP_SET(set, sig, |=);
}

static inline void sigdelset(sigset_t *set, int sig)
{
    SIG_OP_SET(set, sig, &= ~);
}

static inline int sigismember(sigset_t *set, int sig)
{
    return SIG_OP_SET(set, sig, &);
}
#undef	SIG_OP_SET

static inline void sigemptyset(sigset_t *set)
{
    memset(set, '\0', sizeof(*set));
}

static inline void sigfillset(sigset_t *set)
{
    memset(set, '\xff', sizeof(*set));
}

#endif
