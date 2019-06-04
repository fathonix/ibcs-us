/*
 * IBCS used to be a kernel module, and is still written that way.
 * This file does what the Linux 2.6.32 include file <linux/elf.h>
 * did, but does it by emulating the same functions in user user space.
 *
 * These files are mostly derived / copied from the linux kernel source,
 * and so are covered by the same licence.
 *
 * Author: Russell Stuart <russell+ibcs@stuart.id.au>
 */
#ifndef _IBCS_US_LINUX26_INCLUDE_LINUX_ELF_H
#define _IBCS_US_LINUX26_INCLUDE_LINUX_ELF_H
#include <linux/elf.h>

#include <ibcs-us/linux26-compat/linux/mm.h>


#define elfhdr		elf32_hdr
#define elf_phdr	elf32_phdr
#define elf_note	elf32_note
#define elf_addr_t	Elf32_Off

#define ELF_EXEC_PAGESIZE	PAGE_SIZE
#define COMPAT_ELF_PLATFORM	("i686")

#define elf_check_arch(x) \
	(((x)->e_machine == EM_386) || ((x)->e_machine == EM_486))

#define ELF_PLAT_INIT(_r, load_addr)		\
	do {					\
	_r->ebx = 0; _r->ecx = 0; _r->edx = 0;	\
	_r->esi = 0; _r->edi = 0; _r->ebp = 0;	\
	_r->eax = 0;				\
} while (0)

#endif
