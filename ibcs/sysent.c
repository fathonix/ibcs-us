/*
 * This module handles trapping the lcall7 system calls and passing them onto
 * the exec_domain->handler() function.
 */
#include <stdarg.h>
#include <stddef.h>

#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/ibcs/map.h>
#include <ibcs-us/ibcs/sysent.h>
#include <ibcs-us/ibcs/trace.h>
#include <ibcs-us/ibcs/linux26-compat.h>

#include <ibcs-us/linux26-compat/linux/errno.h>
#include <ibcs-us/linux26-compat/linux/kernel.h>
#include <ibcs-us/linux26-compat/linux/mm.h>
#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/sched.h>

#include <bits/sigset.h>
#define	_SIGNAL_H		/* We don't want glibc'c include/signal.h */
#define __USE_GNU		/* for mcontext->gregs[REG_XXX] definitions */
#include <sys/ucontext.h>


/*
 * Translate a host errno to the emulated errno.
 */
int iABI_errors(int errno)
{
    return map_value(current_thread_info()->exec_domain->err_map, errno, 1);
}


/*
 * Make a format spec for syscall so we can trace it.
 */
static void printk_syscall(const struct sysent* ap, unsigned* sp)
{
    char*		a;
    char*		f;
    char		fmt[128 + strlen(ap->se_name)];
    static char		fmt_chars[] = "dop?xs";
    static char*	fmt_fmt[] = {"%d","%o","0x%08p","0x%x","0x%x","\"%.300s\""};

    f = strcpy(fmt, ap->se_name);
    f = strchr(f, '\0');
    *f++ = '(';
    if (ap->se_args) {
	for (a = ap->se_args; *a; a += 1) {
	    if (f[-1] != '(') {
		strcpy(f, ", ");
		f = strchr(f, '\0');
	    }
	    const char* c = strchr(fmt_chars, *a);
	    if (c) {
		strcpy(f, fmt_fmt[c - fmt_chars]);
		f = strchr(f, '\0');
	    } else {
		*f++ = '?';
		*f++ = *a;		/* Repeated 'cause it might be a % */
		*f++ = *a;
		*f++ = '?';
	    }
	}
    }
    strcpy(f, ")\n");
    vprintk(fmt, (va_list)sp);
}

/*
 *	lcall7_syscall    -    indirect syscall for the lcall7 entry point
 *
 *	@regs:		saved user registers
 *
 *	This function implements syscall(2) in kernelspace for the lcall7-
 *	based personalities.
 */
void lcall7_syscall(struct pt_regs *regs)
{
    __get_user(_AX(regs), ((unsigned long *)_SP(regs)) + 1);
    _SP(regs) += sizeof(_AX(regs));
    current_thread_info()->exec_domain->handler(-1, regs);
    _SP(regs) -= sizeof(_AX(regs));
}

/*
 * The exec_domain->handler() all call here.
 */
void lcall7_dispatch(struct pt_regs *regs, struct sysent *ap, int off)
{
    if (ap->se_nargs == Unimpl) {
	abi_printk(ABI_TRACE_UNIMPL, "Unimplemented syscall %S\n");
	set_error(regs, iABI_errors(ENOSYS));
	return;
    }
    void* syscall = ap->se_syscall;
    if (syscall == (void*)0) {
	abi_printk(ABI_TRACE_UNIMPL, "Unknown syscall %S\n");
	set_error(regs, iABI_errors(ENOSYS));
	return;
    }
    /*
     * Trace.
     */
    unsigned* sp = ((unsigned*)_SP(regs)) + off;
    if (abi_traced(ABI_TRACE_API)) {
	printk_syscall(ap, sp);
    }
    /*
     * There are a number of calling conventions defined in ibcs-sysent.h.
     */
    int error;
    if ((size_t)syscall < 512) {
	/*
	 * You are allowed to pass syscall numbers directly.   This works on
	 * 32 bit because the syscall numbering is mostly the same, but on
	 * amd64 they vary a lot.
	 */
        error = (int)linux26_syscall(
	    (int)syscall, sp[0], sp[1], sp[2], sp[3], sp[4], sp[5]);
    }
    else if (ap->se_nargs <= 6) {
        /*
	 * The C calling convention.
	 */
	error = ((u32 (*)(int, int, int, int, int, int))syscall)(
	    sp[0], sp[1], sp[2], sp[3], sp[4], sp[5]);
    } else if (ap->se_nargs == Fast) {
	/*
	 * Pass regs, the error return is already set up.
	 */
	((void (*)(struct pt_regs*))syscall)(regs);
	return;
    } else if (ap->se_nargs == Spl) {
	/*
	 * Pass the regs.
	 */
	error = ((int (*)(struct pt_regs*))syscall)(regs);
    } else {
	abi_printk(
	    ABI_TRACE_API, "%s unrecognised calling convention %d\n",
	    ap->se_name, ap->se_nargs);
	error = -ENOSYS;
    }
    /*
     * Now format that return value as the lcall calling convetion expects.
     * To wit: if carry is set AX contains an errno, otherwise AX contains
     * the return value.
     */
    if (IBCS_IS_ERR(error)) {
	set_error(regs, iABI_errors(-error));
	abi_printk(
	    ABI_TRACE_API, "%s error return %d\n", ap->se_name, _AX(regs));
    } else {
	clear_error(regs);
	set_result(regs, error);
	abi_printk(
	    ABI_TRACE_API, "%s returns %d (edx:%ld)\n",
	    ap->se_name, _AX(regs), _AX(regs), _DX(regs));
    }
}


/*
 * The binfmt loaders (eg, coff) call this instead of do_mmap().  They expect
 * it to behave identically (because in the kernel version they did just
 * call do_mmap()).  This gives us a chance to ensure we can write to the
 * created .text segments.
 */
unsigned long binfmt_mmap(
    struct file* file, unsigned long addr,
    unsigned long len, unsigned long prot,
    unsigned long flag, unsigned long offset
) {
    unsigned long	mmap_prot = prot;
    unsigned long	mmap_flag = flag;
    if (prot & PROT_EXEC && (!(prot & PROT_READ) || !(prot & PROT_WRITE))) {
        mmap_prot |= PROT_READ | PROT_WRITE;
	mmap_flag &= ~MAP_DENYWRITE;
    }
    unsigned long result = do_mmap(file, addr, len, mmap_prot, mmap_flag, offset);
    /*
     * The brk() effectively adjusts the .bss mmap().   The .bss mmap()
     * doesn't have a file, so if this doesn't have a file record it.
     */
    if (!IBCS_IS_ERR(result)) {
	if (file == (struct file*)0 && current->mm->ibcs_bss_mmap_addr < result) {
	    current->mm->ibcs_bss_mmap_addr = result;
	    current->mm->ibcs_bss_mmap_len = len;
	}
	if (current->mm->ibcs_high_mmap_addr < result) {
	    current->mm->ibcs_high_mmap_addr = result;
	    current->mm->ibcs_high_mmap_len = len;
	}
    }
    return result;
}


/*
 * Entry point for lcall syscalls.
 *
 * Since the code we are about to call was written for a linux-2.6.32 kernel
 * it expects things to set up as the linux 32 bit entry point would do it.
 * That is:
 *
 * -   All user registers (ie, the contents of the registers just prior to
 *     the syscall instruction being executed) pushed onto the stack so
 *     they form a "struct pt_regs".
 *
 * -   The domain entry point handler is called like this:
 *         (*current->exec_domain->handler)(lcall_gate, pt_regs)
 *     where:
 *         lcall_gate	Is the call gate used (7 or 27).
 *         pt_regs	Pointer to the struct pt_regs on the stack.
 *
 * -   On return the return value is ignored.  The (probably modified)
 *     contents of the struct pt_regs is loaded into the user registers.
 *     The linux-2.6.32 kernel would restore everything, modifing
 *     ss:esp or cs:eip only happened in circumstaces we don't have to
 *     handle, so we don't restore those.
 *
 * We have been called by the code installed by lcall_sigsegv() below.  That
 * code is:
 *
 * 	push	lcall_gate
 * 	call	ibcs_lcall
 *
 * So the stack is:
 *
 *      esp+0:	return_ip
 *      esp+4:	lcall_gate
 *      esp+8:	syscall_arg_6
 *
 * On return esp must be exactly as it was before the above sequence was
 * executed, ie pointing to syscall_arg_6.  So we mangle the stack to look
 * like this when the handler is called:
 *
 *   	%esp-76:	lcall_gate
 *   	%esp-72:	[%esp-68]
 *   	%esp-68:	struct pt_regs {...}
 *   	%esp+0:		arg-6		<-- pt_regs.esp must point here
 *   	%esp+12:	arg-7
 */
extern int ibcs_lcall(void);

void _ibcs_lcall_dummy()
{
    /*
     * We are playing with fire here.  gcc must not touch any registers before
     * we load them, so we bypass the gcc entry and exit code by embedding 
     * the pure assembler ibcs_lcall() in a dummy gcc function.
     *
     * The first step is to allocate a struct pt_regs on the stack and
     * save all the registers to it.  The code below much match the C
     * struct pt_regs definition, which you can find here (you are after
     * the __i386__ one):
     *
     *     /usr/include/i386-linux-gnu/asm/ptrace.h
     */
    asm volatile (
	"ibcs_lcall:\n"
	"\n"
	"lea	-60(%%esp),%%esp\n"	/* %esp -= sizeof(struct pt_regs) - sizeof(return_ip) - sizeof(lcall_gate) */
	"movl	%%ebx,0(%%esp)\n"	/* pt_regs.ebx = %ebx */
	"movl	%%ecx,4(%%esp)\n"	/* pt_regs.ecx = %ecx */
	"movl	%%edx,8(%%esp)\n"	/* pt_regs.edx = %edx */
	"movl	%%esi,12(%%esp)\n"	/* pt_regs.esi = %esi */
	"movl	%%edi,16(%%esp)\n"	/* pt_regs.edi = %edi */
	"movl	%%ebp,20(%%esp)\n"	/* pt_regs.ebp = %ebp */
	"movl	%%eax,24(%%esp)\n"	/* pt_regs.eax = %eax */
	"movl	%%ds,%%edx\n"
	"movl	%%edx,28(%%esp)\n"	/* pt_regs.xds = %ds */
	"movl	%%es,%%edx\n"
	"movl	%%edx,32(%%esp)\n"	/* pt_regs.xes = %es */
	"movl	%%fs,%%edx\n"
	"movl	%%edx,36(%%esp)\n"	/* pt_regs.xfs = %fs */
	"movl	%%gs,%%edx\n"
	"movl	%%edx,40(%%esp)\n"	/* pt_regs.xgs = %gs */
	"movl	%%eax,44(%%esp)\n"	/* pt_regs.orig_ax = %eax */
	"movl	60(%%esp),%%edx\n"
	"movl	%%edx,48(%%esp)\n"	/* pt_regs.eip */
         /*
	  * Take a break from saving registers to pt_regs to rearrange
	  * the stack are about to overwrite.  This will destroy lcall_gate,
	  * to save that in %ecx.
	  */
	"movl	64(%%esp),%%ebp\n"	/* %ebp = lcall_gate */
	/*
	 * Save the remaining registers to pt_regs.
	 */
	"movl	%%cs,%%edx\n"
	"movl	%%edx,52(%%esp)\n"	/* pt_regs.xcs */
	"pushf\n"
	"popl	%%edx\n"
	"movl	%%edx,56(%%esp)\n"	/* pt_regs.eflags */
	"lea	68(%%esp),%%edx\n"
	"movl	%%edx,60(%%esp)\n"	/* pt_regs.esp */
	"movl	%%ss,%%edx\n"
	"movl	%%edx,64(%%esp)\n"	/* pt_regs.xss */
	:
    );
    /*
     * How everything is saved we can dare to call some gcc functions that
     * smash registers.  We need one: the one that allows us to get to
     * the static variable current->exec_domain->handler.  Gcc set this up
     * in the _ibcs_lcall_dummy() prelude, but we skipped that.
     */
    asm volatile (
	"call	__x86.get_pc_thunk.ax\n"
	"addl	$_GLOBAL_OFFSET_TABLE_,%%eax\n"
	:
    );
    /*
     * Now gcc's GOT pointer is set up we can ask it to load "current" for us.
     */
    asm volatile (
	"movl	%%esp,%%edx\n"		/* %edx = &struct pt_regs */
	"pushl	%%edx\n"		/* arg2: struct pt_regs* regs */
	"pushl	%%ebp\n"		/* arg1: int lcall_gate */
	"call	*%0\n"			/* (*%eax)(lcall_gate, regs) */
	"addl	$8,%%esp\n"
	:
	: "a" (current->exec_domain->handler)
    );
    /*
     * The handler has set the struct pt_regs we passed him to contain the
     * registers as he wants the caller to see them.
     */
    asm volatile (
	/*
	 * We don't restore ss:sp.  ss should never change in a user space
	 * program.  If someone wants to mess with esp they will have to
	 * either handle the return themselves or fake it so we work.
	 *
	 * The odd handling of eip is because it becomes the return address,
	 * so it's place on the stack where "ret" below will find it and after
	 * excuting will leave esp as we found it in ibcs_sigsegv().
	 */
	"pushl	56(%%esp)\n"
	"popf\n"			/* %eflags = pt_regs.eflags */
#if 0					/* cs must be loaded with retf */
	"movl	52(%%esp),%%edx\n"	/* pt_regs.xcs */
	"mov	%%dx,%%cs\n"
#endif
	"movl	48(%%esp),%%edx\n"
	"movl   %%edx,64(%%esp)\n"	/* pt_regs.xss = pt_regs.eip */
	/* Index 44 is pt_regs.orig_ax, which is ignored */
	"movl	40(%%esp),%%edx\n"	/* %gs = pt_regs.xes */
	"movw	%%dx,%%gs\n"
	"movl	36(%%esp),%%edx\n"	/* %fs = pt_regs.xes */
	"movw	%%dx,%%fs\n"
	"movl	32(%%esp),%%edx\n"	/* %es = pt_regs.xes */
	"movw	%%dx,%%es\n"
	"movl	28(%%esp),%%edx\n"	/* %ds = pt_regs.xds */
	"movw	%%dx,%%ds\n"
	"movl	28(%%esp),%%edx\n"
	"movl	24(%%esp),%%eax\n"	/* %eax = pt_regs.eax */
	"movl	20(%%esp),%%ebp\n"	/* %ebp = pt_regs.ebp */
	"movl	16(%%esp),%%edi\n"	/* %edi = pt_regs.edi */
	"movl	12(%%esp),%%esi\n"	/* %esi = pt_regs.esi */
	"movl	8(%%esp),%%edx\n"	/* %edx = pt_regs.edx */
	"movl	4(%%esp),%%ecx\n"	/* %ecx = pt_regs.ecx */
	"movl	0(%%esp),%%ebx\n"	/* %ebx = pt_regs.ebx */
	:
    );
    /*
     * Finally, remove the struct pt_regs and the old ip from the stack
     * and return.
     */
    asm volatile (
	"lea	64(%%esp),%%esp\n"	/* %esp += sizeof(*pt_regs) - sizeof(return_ip) */
	"ret"
	:
    );
}


/*
 * This is the mechanism we use to trap lcall7 syscalls.  A much better
 * way (better as in faster, safer because the code segment could be left
 * !PROT_WRITE and neater to boot) would be to use the kernels' modify_ldt
 * call to create a call gate, but as it happens modify_ldt doesn't know
 * what a call gate it so it always fails.  So instead we reply on the
 * lcall7's causing a SIGSEGV, and use self modifying code. Ick, ick, ick.
 *
 * As SIGSEGV's don't normally happen this is almost certainly is a lcall
 * instruction, which is a syscall we have to intercept.
 *
 * If it is we modify the binary to transfer control to our intercept, and
 * continue.  Linux will then restart the program at the IP address that
 * caused the SIGSEGV, which will immediately execute the modified
 * instructions.  The modified instructions push the gate number used, and
 * then call the ibcs_lcall function above.
 *
 * If it isn't a lcall7 we do our best to make it look like we weren't here
 * at all.
 */
static void lcall_sigsegv(int signal, siginfo_t* siginfo, void* context)
{
    const ucontext_t*	uc = context;
    size_t		ip = uc->uc_mcontext.gregs[REG_EIP];
    const size_t	call_instr_len = 5;

    if (*(unsigned char*)ip == 0x9a && *(unsigned*)(ip + 1) == 0) { /* lcall ? */
	const short	lcall_gate = *(short*)(ip + 5);
	if (lcall_gate == 7 || lcall_gate == 27) {
	    /*
	    * This patching is only possible because the binfmt loaders used
	    * binfmt_mmap above, which ensure we are allowed to write it.
	    * Normally .text segments don't allow writes.
	    *
	    * This changes:
	    *   9a 00 00 00 00 07 00	lcall  $0x7,$0x0
	    * To:
	    *   6a 07                   push $7
	    *   e8 xx xx xx xx		call ibcs_lcall
	    */
	    *(unsigned short*)(ip + 0) = (lcall_gate << 8) | 0x6a; /* push lcall_gate */
	    *(unsigned char*)(ip + 2) = 0xe8;	/* call instruction */
	    *(unsigned*)(ip + 3) = (unsigned)ibcs_lcall - ip - 2 - call_instr_len;
	    return;
	}
    }
    /*
     * Errk, it's not an lcall.  It must be a real SIGSEGV.  Pretend we
     * weren't here.
     */
    struct sigaction*	sa = &current->_linux26_sigtab[SIGSEGV];
    if (sa->sa_flags & SA_SIGINFO) {
        typedef void (*siginfo_function_ptr)(int, struct siginfo*, void*);
	((siginfo_function_ptr)sa->sa_handler)(signal, siginfo, context);
    } else if (sa->sa_handler == SIG_IGN) {
        ;
    } else if (sa->sa_handler != SIG_DFL) {
	sa->sa_handler(signal);
    } else {
        /*
	 * The most likely outcome - they didn't do anything special.
	 * So just make it happen again, but let nature take it's course.
	 */
        ibcs_sigaction(SIGSEGV, sa, (struct sigaction*)0);
    }
}


/*
 * Trap the SIGSEGV's the lcall's generate, and patch them.
 */
void sysent_initialise()
{
    int			ret;
    struct sigaction	sa;

    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = (__sighandler_t)lcall_sigsegv;
    sa.sa_flags = SA_SIGINFO;
    memset(&sa.sa_mask, ~0, sizeof(sa.sa_mask));
    ret = ibcs_sigaction(SIGSEGV, &sa, (struct sigaction*)0);
    if (IBCS_IS_ERR(ret)) {
	ibcs_fatal_syscall(ret, "sysent_initialise sigaction(%d)", SIGSEGV);
    }
}
