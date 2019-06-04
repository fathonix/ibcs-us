
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
#ifndef _IBCS_US_SVR4_Y2K_H
#define _IBCS_US_SVR4_Y2K_H

#include <ibcs-us/linux26-compat/linux/personality.h>
#include <ibcs-us/linux26-compat/linux/sched.h>

#define SVR4_Y2K_SHIFT	(24 * 60 * 60)		/* Seconds in a day */
#define SVR4_Y2K_FEB29	(11015 * 24 * 60 *60)	/* Seconds since 01/01/70 */


static inline time_t svr4_y2k_send(time_t secs)
{
    return current->personality & ABI_Y2K_BUG && secs > SVR4_Y2K_FEB29 ?
	secs - SVR4_Y2K_SHIFT : secs;
}


static inline time_t svr4_y2k_recv(time_t secs)
{
    return current->personality & ABI_Y2K_BUG && secs > SVR4_Y2K_FEB29 - SVR4_Y2K_SHIFT ?
	secs + SVR4_Y2K_SHIFT : secs;
}

#endif
