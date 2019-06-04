/*
 *  Copyright (C) 1993  Joe Portman (baron@hebron.connected.com)
 *	 First stab at ulimit
 *
 *  April 9 1994, corrected file size passed to/from setrlimit/getrlimit
 *    -- Graham Adams (gadams@ddrive.demon.co.uk)
 *
 */
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/capability.h>
#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/module.h>
#include <ibcs-us/linux26-compat/linux/resource.h>
#include <ibcs-us/linux26-compat/linux/sched.h>


/*
 * Arguments to ulimit - it's one of the stupid multipled calls...
 */
#define U_GETFSIZE 	(1)		  /* get max file size in blocks */
#define U_SETFSIZE 	(2)		  /* set max file size in blocks */
#define U_GETMEMLIM	(3)		  /* get process size limit */
#define U_GETMAXOPEN	(4)		  /* get max open files for this process */
#define U_GTXTOFF		(64)		  /* get text offset */

/*
 * Define nominal block size parameters.
 */
#define ULIM_BLOCKSIZE_BITS   9           /* block size = 512 */
#define ULIM_MAX_BLOCKSIZE (INT_MAX >> ULIM_BLOCKSIZE_BITS)


int
svr4_ulimit (int cmd, int val)
{
  	unsigned long lim_cur;
	struct rlimit *rlim;
#ifdef	_KSL_IBCS_US
	struct rlimit rlim_struct;
	rlim = &rlim_struct;
#else
	struct rlimit current_rlim;
#if _KSL < 10
	current_rlim=current->rlim;
#else
	current_rlim=current->signal->rlim;
#endif
#endif
	switch (cmd) {
	case U_GETFSIZE:
#ifndef	_KSL_IBCS_US
	        rlim = &current_rlim[RLIMIT_FSIZE];
#else
		SYS(getrlimit, RLIMIT_FSIZE, rlim);
#endif
		return rlim->rlim_cur >> ULIM_BLOCKSIZE_BITS;

	case U_SETFSIZE:
#ifndef	_KSL_IBCS_US
	        rlim = &current_rlim[RLIMIT_FSIZE];
#else
		SYS(getrlimit, RLIMIT_FSIZE, rlim);
#endif
		if ((val > ULIM_MAX_BLOCKSIZE) || (val < 0))
			return -ERANGE;
		lim_cur = val;
		lim_cur <<= ULIM_BLOCKSIZE_BITS;
		if (lim_cur > rlim->rlim_max) {
			if (!capable(CAP_SYS_RESOURCE))
				return -EPERM;
			rlim->rlim_max = lim_cur;
		}
		rlim->rlim_cur = lim_cur;
#ifdef	_KSL_IBCS_US
		SYS(setrlimit, RLIMIT_FSIZE, rlim);
#endif
		return 0;

	case U_GETMEMLIM:
#ifndef	_KSL_IBCS_US
	        rlim = &current_rlim[RLIMIT_DATA];
#else
		SYS(getrlimit, RLIMIT_DATA, rlim);
#endif
		return rlim->rlim_cur;

	case U_GETMAXOPEN:
#ifndef	_KSL_IBCS_US
	        rlim = &current_rlim[RLIMIT_NOFILE];
#else
		SYS(getrlimit, RLIMIT_NOFILE, rlim);
#endif
		return rlim->rlim_cur;

	default:
#if defined(CONFIG_ABI_TRACE)
		abi_trace(ABI_TRACE_API, "unsupported ulimit call %d\n", cmd);
#endif
		return -EINVAL;
	}
}

/*
 * getrlimit/setrlimit args.
 */
#define U_RLIMIT_CPU	0
#define U_RLIMIT_FSIZE	1
#define U_RLIMIT_DATA	2
#define U_RLIMIT_STACK	3
#define U_RLIMIT_CORE	4
#define U_RLIMIT_NOFILE	5
#define U_RLIMIT_AS	6


int
svr4_getrlimit(int cmd, void *val)
{
int err;
	switch (cmd) {
	case U_RLIMIT_CPU:
		cmd = RLIMIT_CPU;
		break;
	case U_RLIMIT_FSIZE:
		cmd = RLIMIT_FSIZE;
		break;
	case U_RLIMIT_DATA:
		cmd = RLIMIT_DATA;
		break;
	case U_RLIMIT_STACK:
		cmd = RLIMIT_STACK;
		break;
	case U_RLIMIT_CORE:
		cmd = RLIMIT_CORE;
		break;
	case U_RLIMIT_NOFILE:
		cmd = RLIMIT_NOFILE;
		break;
	case U_RLIMIT_AS:
		cmd = RLIMIT_AS;
		break;
	default:
		return -EINVAL;
	}

	err = SYS(getrlimit,cmd, val); return err;
}

int
svr4_setrlimit(int cmd, void *val)
{
int err;
	switch (cmd) {
	case U_RLIMIT_CPU:
		cmd = RLIMIT_CPU;
		break;
	case U_RLIMIT_FSIZE:
		cmd = RLIMIT_FSIZE;
		break;
	case U_RLIMIT_DATA:
		cmd = RLIMIT_DATA;
		break;
	case U_RLIMIT_STACK:
		cmd = RLIMIT_STACK;
		break;
	case U_RLIMIT_CORE:
		cmd = RLIMIT_CORE;
		break;
	case U_RLIMIT_NOFILE:
		cmd = RLIMIT_NOFILE;
		break;
	case U_RLIMIT_AS:
		cmd = RLIMIT_AS;
		break;
	default:
		return -EINVAL;
	}

	err = SYS(getrlimit,cmd, val); return err;
}

EXPORT_SYMBOL(svr4_getrlimit);
EXPORT_SYMBOL(svr4_setrlimit);
EXPORT_SYMBOL(svr4_ulimit);
