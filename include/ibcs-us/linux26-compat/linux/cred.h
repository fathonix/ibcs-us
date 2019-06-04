/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/cred.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_CRED_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_CRED_H
#include <sys/cdefs.h>

#include <ibcs-us/ibcs/ibcs-lib.h>

struct cred;

/*
 * A replacement for current->cred->uid.
 */
static inline uid_t current_cred_uid()
{
    return (uid_t)IBCS_SYSCALL(getuid32);
}

#endif
