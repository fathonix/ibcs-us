#define __KERNEL_SYSCALLS__

#include <ibcs-us/per-cxenix/signal.h>
#include <ibcs-us/ibcs/map.h>
#include <ibcs-us/ibcs/linux26-compat.h>
#include <ibcs-us/per-svr4/signal.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/sched.h>
#include <ibcs-us/linux26-compat/linux/signal.h>

#ifndef	_KSL_IBCS_US
unsigned long abi_sigret(int);
#endif

int
xnx_sigaction(int sco_signum, const struct sco_sigaction *action,
		struct sco_sigaction *oldaction)
{
	struct sco_sigaction	new_sa, old_sa;
	struct sigaction	nsa, osa;
	mm_segment_t		fs;
	int			error, signo;

	if (sco_signum >= NSIGNALS)
		return -EINVAL;
	signo = current_thread_info()->exec_domain->signal_map[sco_signum];

	if (oldaction) {
		if (!access_ok(VERIFY_WRITE, oldaction,
				sizeof(struct sco_sigaction)))
			return -EFAULT;
	}

	if (action) {
		error = copy_from_user(&new_sa, action,
				sizeof(struct sco_sigaction));
		if (error)
			return -EFAULT;
		nsa.sa_handler = new_sa.sco_sa_handler;
		nsa.sa_mask = map_sigvec_to_kernel(new_sa.sco_sa_mask,
			current_thread_info()->exec_domain->signal_map);
		nsa.sa_flags = SA_NOMASK | SA_SIGINFO;
		if (new_sa.sco_sa_flags & SCO_SA_NOCLDSTOP)
			nsa.sa_flags |= SA_NOCLDSTOP;
#ifdef	_KSL_IBCS_US
		nsa.sa_restorer = (void*)0;
#else
		nsa.sa_restorer = (void *) abi_sigret(__NR_IBCS_rt_sigaction);
		if(nsa.sa_restorer) nsa.sa_flags |= SA_RESTORER;
#endif
	}

	fs = get_fs();
	set_fs(get_ds());
	error = SYS_NATIVE(rt_sigaction,signo, action ? &nsa : NULL,
			oldaction ? &osa : NULL, sizeof(sigset_t));
	set_fs(fs);

	if (error || !oldaction)
		return (error);

	old_sa.sco_sa_handler = osa.sa_handler;
	old_sa.sco_sa_mask = map_sigvec_from_kernel(osa.sa_mask,
			current_thread_info()->exec_domain->signal_invmap);
	old_sa.sco_sa_flags = 0;
	if (osa.sa_flags & SA_NOCLDSTOP)
		old_sa.sco_sa_flags |= SCO_SA_NOCLDSTOP;

	if (copy_to_user(oldaction, &old_sa, sizeof(struct sco_sigaction)))
		return -EFAULT;
	return 0;
}

int
xnx_sigpending(u_long *setp)
{
	sigset_t		lxpending;
	u_long			pending;

#ifdef	_KSL_IBCS_US
	int ret = SYS(sigpending, &lxpending);
	if (ret < 0) {
		return ret;
	}
#else
	spin_lock_irq(&current->sighand->siglock);
	sigandsets(&lxpending, &current->blocked, &current->pending.signal);
	spin_unlock_irq(&current->sighand->siglock);
#endif

	pending = map_sigvec_from_kernel(lxpending,
			current_thread_info()->exec_domain->signal_invmap);

	if (copy_to_user(setp, &pending, sizeof(u_long)))
		return -EFAULT;
	return 0;
}
