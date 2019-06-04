/*
 * Copyright (c) 2001 Christoph Hellwig.
 * All rights resered.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef _IBCS_US_IBCS_LIB_TRACE_H
#define _IBCS_US_IBCS_LIB_TRACE_H
#include <stddef.h>

/*
 * Tracing flags.
 */
enum abi_trace_flags
{
    ABI_TRACE_API =	0x00000001,	/* all call/return values	*/
    ABI_TRACE_IOCTL =	0x00000002,	/* all ioctl calls		*/
    ABI_TRACE_IOCTL_F =	0x00000004,	/* ioctl calls that fail	*/
    ABI_TRACE_SIGNAL =	0x00000008,	/* all signal calls		*/
    ABI_TRACE_SIGNAL_F= 0x00000010,	/* signal calls that fail	*/
    ABI_TRACE_SOCKSYS =	0x00000020,	/* socksys and spx devices	*/
    ABI_TRACE_STREAMS =	0x00000040,	/* STREAMS faking		*/
    ABI_TRACE_UNIMPL =	0x00000080,	/* unimplemened functions	*/
    ABI_TRACE_SINODE =	0x00000100,	/* short inode mapping		*/
    ABI_TRACE_FILEMAP =	0x00000200,	/* file name mapping		*/
    ABI_TRACE_ALWAYS =	~0U,		/* unconditionally print trace  */
};

extern int abi_trace_fd;
extern enum abi_trace_flags abi_trace_flg;

#define abi_trace(res, fmt...)	_abi_printk((res), fmt)
#define __abi_trace(fmt...) _abi_printk(ABI_TRACE_ALWAYS, fmt)

#ifndef	CONFIG_ABI_TRACE
#define	abi_printk(res, fmt...)
#else
#define	abi_printk(res, fmt...)	_abi_printk((res), fmt)
#endif

extern void plist(char *, char *, int *);
extern int pdump(char *, int);
extern void _abi_printk(enum abi_trace_flags when, const char* fmt, ...);

/*
 * Check if a syscall needs tracing.
 */
static inline int abi_traced(enum abi_trace_flags when)
{
    return (abi_trace_flg & when) != 0;
}
#endif
