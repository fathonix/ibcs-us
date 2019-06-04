/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/binfmts.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_BINFMTS_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_BINFMTS_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/asm/ptrace.h>
#include <ibcs-us/linux26-compat/linux/fs.h>
#include <ibcs-us/linux26-compat/linux/list.h>
#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/module.h>

#define	BINPRM_BUF_SIZE		128

/*
 * Stack area protections
 * */
#define EXSTACK_DEFAULT		0	/* Whatever the arch defaults to */
#define EXSTACK_DISABLE_X	1	/* Disable executable stacks */
#define EXSTACK_ENABLE_X	2	/* Enable executable stacks */

/*
 * This structure is used to hold the arguments that are used when loading binaries.
 */
struct linux_binprm
{
    int			argc;
    char		buf[BINPRM_BUF_SIZE];
    struct file*	file;
    struct mm_struct *	mm;
    unsigned long	p;	/* current top of mem */
    int			envc;
};

/*
 * This structure defines the functions that are used to load the binary formats that
 * linux accepts.
 */
struct linux_binfmt
{
      struct list_head	lh;
      struct module*	module;
      int		(*load_binary)(struct linux_binprm*, struct pt_regs* regs);
      int		(*load_shlib)(struct file *);
      int		(*core_dump)(long signr, struct pt_regs *regs, struct file *file, unsigned long limit);
      unsigned long	min_coredump;	/* minimal dump size */
      int		hasvdso;
};

extern int register_binfmt(struct linux_binfmt* binfmt);
extern void unregister_binfmt(struct linux_binfmt* binfmt);


static inline int flush_old_exec(struct linux_binprm* bprm)
{
    return 0;
}

static inline void set_binfmt(struct linux_binfmt *new)
{
}

static inline void setup_new_exec(struct linux_binprm* bprm)
{
}

static inline int setup_arg_pages(
    struct linux_binprm * bprm, unsigned long stack_top, int executable_stack
) {
    return 0;
} 

static inline void install_exec_creds(struct linux_binprm *bprm)
{
}
#endif
