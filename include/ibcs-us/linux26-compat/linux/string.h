/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/string.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_STRING_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_STRING_H
#include <sys/cdefs.h>

/*
 * Just use the gcc's builtin's.
 */
#define	memchr(s, b, c)		__builtin_memchr(s, b, c)
#define	memcmp(s1, s2, c)	__builtin_memcmp(s1, s2, c)
#define	memcpy(d, s, c)		__builtin_memcpy(d, s, c)
#define	memset(d, b, c)		__builtin_memset(d, b, c)
#define	strcasecmp(c1, c2)	__builtin_strcasecmp(c1, c2)
#define	strcat(d, s)		__builtin_strcat(d, s)
#define	strchr(s, c)		__builtin_strchr(s, c)
#define	strcmp(c1, c2)		__builtin_strcmp(c1, c2)
#define	strcpy(d, s)		__builtin_strcpy(d, s)
#define	strlen(s)		__builtin_strlen(s)
#define	strncasecmp(c1, c2, c)	__builtin_strncasecmp(c1, c2, c)
#define	strncat(d, s, c)	__builtin_strncat(d, s, c)
#define	strncmp(c1, c2, c)	__builtin_strncmp(c1, c2, c)
#define	strncpy(d, s, c)	__builtin_strncpy(d, s, c)
#define	strnlen(s)		__builtin_strnlen(s)
#define	strrchr(s, c)		__builtin_strrchr(s, c)
#define	strstr(s, n)		__builtin_strstr(s, n)

#define	mempcpy(d, s, c)	__builtin_mempcpy(d, s, c)

#endif
