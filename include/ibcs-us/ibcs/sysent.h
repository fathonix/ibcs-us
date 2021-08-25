#ifndef _IBCS_US_IBCS_SYSENT_H
#define _IBCS_US_IBCS_SYSENT_H
#include <stddef.h>
#include <linux/unistd.h>

#include <ibcs-us/linux26-compat/asm/ptrace.h>
#include <ibcs-us/linux26-compat/asm/uaccess.h>
#include <ibcs-us/linux26-compat/linux/types.h>

/*
 * The change from 32 bit to 64 bit changed the register names.
 *
 * But ibcs-us is 32 bit only now, so this hack could go.
 */
#ifdef CONFIG_64BIT
#define _AX(x) x->rax
#define _BX(x) x->rbx
#define _CX(x) x->rcx
#define _DX(x) x->rdx
#define _SI(x) x->rsi
#define _DI(x) x->rdi
#define _BP(x) x->rbp
#define _SP(x) x->rsp
#define _IP(x) x->rip
#define _CS(x) x->cs
#define _SS(x) x->ss
#define _OAX(x) x->orig_rax
#define _FLG(x) x->eflags

#else

#define _AX(x) x->eax
#define _BX(x) x->ebx
#define _CX(x) x->ecx
#define _DX(x) x->edx
#define _SI(x) x->esi
#define _DI(x) x->edi
#define _BP(x) x->ebp
#define _SP(x) x->esp
#define _IP(x) x->eip
#define _DS(x) x->xds
#define _ES(x) x->xes
#define _CS(x) x->xcs
#define _SS(x) x->xss
#define _FS(x) x->xfs
#define _GS(x) x->xgs
#define _OAX(x) x->orig_eax
#define _FLG(x) x->eflags
#endif /* 64BIT */

/*
 *  - If an entry is 'sysent_call_unknow' we don't know how to handle it yet.
 */
enum
{
    Spl			= 's',		/* pass the regs structure */
    Fast		= 'f',		/* pass regs & errno setup on return */
    Unimpl		= 'u',		/* syscall is not implemented yet */
    Ukn			= 'k',		/* source compat (XXX: kill me!) */
};

/*
 * Every entry in the systen tables is described by this structure.
 */
struct sysent
{
    void*		se_syscall;	/* function to call */
    short		se_nargs;	/* number of aguments */
    /*
    * Theses are only used for syscall tracing.
    */
    char*		se_name;	/* name of function */
    char*		se_args;	/* how to print the argument list */
};


/*
 * This does an emulated SYSCALL.
 */
struct pt_regs;

extern void lcall7_dispatch(struct pt_regs *regs, struct sysent *ap, int off);
extern void lcall7_syscall(struct pt_regs *regs);
extern void sysent_initialise();
extern int iABI_errors(int errno);


/**
 * get_syscall_parameter - get syscall parameter
 * @regs: saved register contents
 * @n: nth syscall to get
 *
 * This function gets the nth syscall parameter if
 * the syscall got a &struct pt_regs * passed instead
 * of the traditional C calling convention.
 */
static inline unsigned long get_syscall_parameter(struct pt_regs *regs, int n)
{
    unsigned int	r;

    __get_user(r, (u32 *)_SP(regs) + (n + 1));
    return (r & 0xFFFFFFFF);
}

/**
 * set_error - set error number
 * @regs: saved register contents
 * @errno: error number
 *
 * This function sets the syscall error return for lcall7
 * calling conventions to @errno.
 */
static inline void set_error(struct pt_regs *regs, int errno)
{
    _AX(regs) = errno;
    _FLG(regs) |= 1;
}

/**
 * clear_error - clear error return flag
 * @regs: saved register contents
 *
 * This funtion clears the flag that indicates an error
 * return for lcall7-based syscalls.
 */
static inline void clear_error(struct pt_regs *regs)
{
    _FLG(regs) &= ~1;
}

/**
 * set_result - set syscall return value
 * @regs: saved register contents
 *
 * This funtion sets the return value for syscalls
 * that have the saved register set calling convention.
 */
static inline void set_result(struct pt_regs *regs, int r)
{
    _AX(regs) = r;
}

/**
 * get_result - get syscall return value
 * @regs: saved register contents
 *
 * This funtion gets the return value for syscalls
 * that have the saved register set calling convention.
 */
static inline int get_result(struct pt_regs *regs)
{
    return _AX(regs);
}
#endif
