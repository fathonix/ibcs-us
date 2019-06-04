/*
 * This module maps inode's to 16 bit numbers, as required by some
 * personalities.
 */
#ifndef _IBCS_US_IBCS_LIB_SHORT_INODE_H
#define _IBCS_US_IBCS_LIB_SHORT_INODE_H
#include <stddef.h>

#include <ibcs-us/linux26-compat/linux/types.h>

extern int short_inode_construct();
extern void short_inode_destruct(void);

extern ino_t short_inode_map(ino_t ino);
#endif
