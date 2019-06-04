/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <asm/page.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_ASM_PAGE_H
#define _IBCS_US_LINUX26_INCLUDE_ASM_PAGE_H
#include <sys/cdefs.h>

#define __pgprot(x)	((pgprot_t) { (x) } )

typedef struct
{
    unsigned long	pgprot;
} pgprot_t;

#endif
