/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <asm/uaccess.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _ASM_UACCESS_H
#define _ASM_UACCESS_H
#include <sys/cdefs.h>

#include <ibcs-us/linux26-compat/asm/processor.h>
#include <ibcs-us/linux26-compat/linux/string.h>

#define VERIFY_READ	0
#define VERIFY_WRITE	1

#define put_user(x, ptr) ({*(ptr) = (x); 0;})
#define get_user(x, ptr) ({(x) = *(ptr); 0;})
#define	__get_user(x, ptr) get_user(x, ptr)
#define	__put_user(x, ptr) put_user(x, ptr)

static inline unsigned long copy_to_user(void *to, const void* from, unsigned long n)
{
    memcpy(to, from, n);
    return 0;
}

static inline unsigned long copy_from_user(void* to, const void* from, unsigned long n)
{
    memcpy(to, from, n);
    return 0;
}

static inline long __copy_from_user(void* to, const void* from, unsigned long n)
{
    return copy_from_user(to, from, n);
}

static inline long __copy_to_user(void* to, const void* from, unsigned long n)
{
    return copy_to_user(to, from, n);
}

static inline void set_fs(mm_segment_t fs)
{
}

static inline unsigned long clear_user(void *mem, unsigned long len)
{
    memset(mem, '\0', len);
    return 0;
}

static inline mm_segment_t get_ds()
{
    mm_segment_t	seg = { 0 };
    return seg;
}

static inline mm_segment_t get_fs()
{
    mm_segment_t	seg = { 0 };
    return seg;
}

static inline int access_ok(int type, const void* addr, unsigned long size)
{
    return 1;
}
#endif
