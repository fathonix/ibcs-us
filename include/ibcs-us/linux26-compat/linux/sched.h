/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/sched.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_SCHED_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_SCHED_H
#include <sys/cdefs.h>

#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/linux26-compat/linux/signal.h>

#define current_thread_info()	current

/*
 * Per process flags
 */
#define PF_KSOFTIRQD	0x00000001	/* I am ksoftirqd */
#define PF_STARTING	0x00000002	/* being created */
#define PF_EXITING	0x00000004	/* getting shut down */
#define PF_EXITPIDONE	0x00000008	/* pi exit done on shut down */
#define PF_VCPU		0x00000010	/* I'm a virtual CPU */
#define PF_FORKNOEXEC	0x00000040	/* forked but didn't exec */
#define PF_MCE_PROCESS  0x00000080      /* process policy on mce errors */
#define PF_SUPERPRIV	0x00000100	/* used super-user privileges */
#define PF_DUMPCORE	0x00000200	/* dumped core */
#define PF_SIGNALED	0x00000400	/* killed by a signal */
#define PF_MEMALLOC	0x00000800	/* Allocating memory */
#define PF_FLUSHER	0x00001000	/* responsible for disk writeback */
#define PF_USED_MATH	0x00002000	/* if unset the fpu must be initialized before use */
#define PF_FREEZING	0x00004000	/* freeze in progress. do not account to load */
#define PF_NOFREEZE	0x00008000	/* this thread should not be frozen */
#define PF_FROZEN	0x00010000	/* frozen for system suspend */
#define PF_FSTRANS	0x00020000	/* inside a filesystem transaction */
#define PF_KSWAPD	0x00040000	/* I am kswapd */
#define PF_OOM_ORIGIN	0x00080000	/* Allocating much memory to others */
#define PF_LESS_THROTTLE 0x00100000	/* Throttle me less: I clean memory */
#define PF_KTHREAD	0x00200000	/* I am a kernel thread */
#define PF_RANDOMIZE	0x00400000	/* randomize virtual address space */
#define PF_SWAPWRITE	0x00800000	/* Allowed to write to swap */
#define PF_SPREAD_PAGE	0x01000000	/* Spread page cache over cpuset */
#define PF_SPREAD_SLAB	0x02000000	/* Spread some slab caches over cpuset */
#define PF_THREAD_BOUND	0x04000000	/* Thread bound to specific cpu */
#define PF_MCE_EARLY    0x08000000      /* Early kill for mce process policy */
#define PF_MEMPOLICY	0x10000000	/* Non-default NUMA mempolicy */
#define PF_MUTEX_TESTER	0x20000000	/* Thread belongs to the rt mutex tester */
#define PF_FREEZER_SKIP	0x40000000	/* Freezer should not count it as freezeable */
#define PF_FREEZER_NOSIG 0x80000000	/* Freezer won't send signals to it */

#define	TASK_COMM_LEN	16

struct exec_domain;

struct task_struct
{
    struct files_struct* files;
    unsigned		flags;
    struct mm_struct*	mm;
    pid_t		pid;
    int			personality;
    int			ptrace;
    struct exec_domain* exec_domain;
    char		comm[TASK_COMM_LEN]; /* executable name excluding path */
    void		(*start_addr)(void);
    struct sigaction	_linux26_sigtab[16 + 1];
};

extern struct task_struct* current;

static inline void send_sig(int signal, struct task_struct* task, int priv)
{
    if (task == current) {
        int pid = IBCS_SYSCALL(getpid);
	IBCS_SYSCALL(kill, pid, signal);
    }
}

extern int linux26_schedule_timeout(long period_in_jiffies);
#endif
