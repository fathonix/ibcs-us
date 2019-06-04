/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/types.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_TYPES_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_TYPES_H
#include <bits/types.h>
#include <linux/types.h>
#include <sys/cdefs.h>

typedef unsigned short	umode_t;

typedef	__s8		s8;
typedef	__u8		u8;

typedef	__s16		s16;
typedef	__u16		u16;

typedef	__s32		s32;
typedef	__u32		u32;

typedef __s64		s64;
typedef __u64		u64;

typedef char *		__kernel_caddr_t;
typedef __u32		__kernel_dev_t;
typedef unsigned short	__kernel_gid16_t;
typedef unsigned int	__kernel_gid32_t;
typedef unsigned long	__kernel_ino_t;
typedef	__s64		__kernel_loff_t;
typedef unsigned short	__kernel_nlink_t;
typedef long		__kernel_off_t;
typedef unsigned short	__kernel_old_gid_t;
typedef unsigned short	__kernel_old_uid_t;
typedef int		__kernel_pid_t;
typedef long		__kernel_time_t;
typedef unsigned short	__kernel_uid16_t;
typedef unsigned int	__kernel_uid32_t;

typedef __u32		__socklen_t;
typedef	__s32		__pid_t;

typedef	__kernel_loff_t	loff_t;
typedef unsigned	gfp_t;

typedef __kernel_caddr_t caddr_t;
typedef __kernel_dev_t	dev_t;
typedef __kernel_ino_t	ino_t;
typedef __kernel_mode_t	mode_t;
typedef __kernel_nlink_t nlink_t;
typedef __kernel_off_t	off_t;
typedef __kernel_old_gid_t old_gid_t;
typedef __kernel_old_uid_t old_uid_t;
typedef __kernel_pid_t	pid_t;
typedef __kernel_ssize_t ssize_t;
typedef __kernel_suseconds_t suseconds_t;
typedef __kernel_time_t	time_t;

typedef _Bool		bool;

typedef __kernel_uid32_t uid_t;
typedef __kernel_gid32_t gid_t;
typedef __kernel_uid16_t uid16_t;
typedef __kernel_gid16_t gid16_t;

typedef unsigned long	uintptr_t;
typedef unsigned	fmode_t;

/* bsd */
typedef unsigned char	u_char;
typedef unsigned short	u_short;
typedef unsigned int	u_int;
typedef unsigned long	u_long;

/* sysv */
typedef unsigned char	unchar;
typedef unsigned short	ushort;
typedef unsigned int	uint;
typedef unsigned long	ulong;

typedef	__u8		u_int8_t;
typedef	__s8		int8_t;
typedef	__u16		u_int16_t;
typedef	__s16		int16_t;
typedef	__u32		u_int32_t;
typedef	__s32		int32_t;

typedef	__u8		uint8_t;
typedef	__u16		uint16_t;
typedef	__u32		uint32_t;

typedef	__u64		uint64_t;
typedef	__u64		u_int64_t;
typedef	__s64		int64_t;

typedef struct {
    unsigned long	fds_bits[__FD_SETSIZE / 8 / sizeof(unsigned long)];
} fd_set;

#define	__user

#define	__FD_SETSIZE	1024

#define __FD_SET(fd, fdsetp)						\
    ({									\
	const unsigned bit = (unsigned)(fd);				\
	const unsigned sz = sizeof(((fd_set*)0)->fds_bits[0]) * 8;	\
	((fd_set*)(fdsetp))->fds_bits[bit / sz] |= 1 << (bit % sz)	\
    })

#define __FD_CLR(fd, fdsetp)						\
    ({									\
	const unsigned bit = (unsigned)(fd);				\
	const unsigned sz = sizeof(((fd_set*)0)->fds_bits[0]) * 8;   	\
	((fd_set*)(fdsetp))->fds_bits[bit / sz] &= ~(1 << (bit % sz)) 	\
    })


#define __FD_ISSET(fd, fdsetp)						\
    ({									\
	const unsigned bit = (unsigned)(fd);				\
	const unsigned sz = sizeof(((fd_set*)0)->fds_bits[0]) * 8;   	\
	((fd_set*)(fdsetp))->fds_bits[bit / sz] & (1 << (bit % sz))  	\
    })

#define __FD_ZERO(fdsetp)	(memset(fdsetp, '\0', sizeof(fd_set)))

#endif
