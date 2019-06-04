#ifndef _IBCS_US_IBCS_LIB_MAP_H
#define _IBCS_US_IBCS_LIB_MAP_H
#include <stddef.h>

#include <ibcs-us/linux26-compat/linux/signal.h>

struct map_segment
{
    int			start;
    int			end;
    unsigned char*	map;
};


extern unsigned short map_flags(unsigned short f, unsigned short map[]);
extern unsigned long map_bitvec(unsigned long vec, unsigned long map[]);
extern unsigned long map_sigvec_from_kernel(sigset_t vec, unsigned long map[]);
extern sigset_t map_sigvec_to_kernel(unsigned long vec, unsigned long map[]);
extern int map_value(struct map_segment* m, int val, int def);

#endif
