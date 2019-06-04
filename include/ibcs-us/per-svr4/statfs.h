#ifndef _IBCS_US_SVR4_STATFS_H
#define _IBCS_US_SVR4_STATFS_H

//#ident "%W% %G%"

#include <ibcs-us/linux26-compat/linux/types.h>


struct svr4_statfs {
	int16_t	f_type;
	int32_t f_bsize;
	int32_t f_frsize;
	int32_t f_blocks;
	int32_t f_bfree;
	int32_t	f_files;
	int32_t f_ffree;
	char f_fname[6];
	char f_fpack[6];
};

#endif /* _IBCS_US_SVR4_STATFS_H */
