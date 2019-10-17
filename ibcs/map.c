/*
 * Copyright (C) 1994  Mike Jagdis (jaggy@purplet.demon.co.uk)
 *
 * Emulations often use slightly different signal and errnos to linux.  This
 * module provides utilities for mapping one to the other via a table provided
 * by the emulation.
 */
#include <ibcs-us/ibcs/map.h>

#include <ibcs-us/linux26-compat/linux/signal.h>


u_short map_flags(u_short f, u_short map[])
{
    u_short		m;
    int			i;
    u_short		r = 0;

    for (i = 0, m = 1; i < 16; i += 1) {
	if (f & m) {
	    r |= map[i];
	}
	m <<= 1;
    }
    return r;
}

u_long map_bitvec(u_long vec, u_long map[])
{
    int			i;
    u_long		newvec = 0;
    u_long		m = 1;

    for (i = 1; i <= 32; i++) {
	if ((vec & m) && map[i] != -1 && map[i] != 0) {
	    newvec |= (1 << (map[i] - 1));
	}
	m <<= 1;
    }
    return newvec;
}


u_long map_sigvec_from_kernel(sigset_t vec, u_long map[])
{
    int			i;
    u_long		newvec = 0;

    for (i = 1; i <= 32; i += 1) {
	if (sigismember(&vec, i) && map[i] != -1 && map[i] != 0) {
	    newvec |= (1 << (map[i] - 1));
	}
    }
    return newvec;
}


sigset_t map_sigvec_to_kernel(u_long vec, u_long map[])
{
    int			i;
    sigset_t		newvec;
    u_long		m = 1;

    sigemptyset(&newvec);
    for (i = 1; i <= 32; i++) {
	if ((vec & m) && map[i] != -1) {
	    sigaddset(&newvec, map[i]);
	}
	m <<= 1;
    }
    return newvec;
}


int map_value(struct map_segment* m, int val, int def)
{
    struct map_segment*	seg;

    /*
    * If no mapping exists in this personality just return the
    * number we were given.
    */
    if (!m) {
	return val;
    }
    /*
    * Search the map looking for a mapping for the given number.
    */
    for (seg = m; seg->start != -1; seg++) {
	if (seg->start <= val && val <= seg->end) {
	    /*
	    * If the start and end are the same then this
	    * segment has one entry and the map is the value
	    * it maps to. Otherwise if we have a vector we
	    * pick out the relevant value, if we don't have
	    * a vector we give identity mapping.
	    */
	    if (seg->start == seg->end) {
		return (int)(long)seg->map;
	    }
	    return seg->map ? seg->map[val - seg->start] : val;
	}
    }
    /* Number isn't mapped. Returned the requested default. */
    return def;
}
