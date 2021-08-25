/*
 * Mapping file names in the emulation.
 *
 * Copyright (c) 2019,2021 Russell Stuart <russell+ibcs-us@stuart.id.au>
 * Licensed under the same terms as the Linux 2.6.32 kernel.
 */
#ifndef _IBCS_US_IBCS_FILEMAP_H
#define	_IBCS_US_IBCS_FILEMAP_H
#include <stddef.h>

extern int filemap_parse(const char* map_filename);
extern int filemap_map(char* dst, size_t dst_size, const char* path);
#endif
