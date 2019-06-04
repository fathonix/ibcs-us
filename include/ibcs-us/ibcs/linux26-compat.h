/*
 * Stuff to make userspace look like the linux-2.6.26 kernel API.
 *
 * License
 * -------
 *
 * Copyright (c) 2019 Russell Stuart <russell+ibcs@stuart.id.au>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or (at
 * your option) any later version.
 *
 * The copyright holders grant you an additional permission under Section 7
 * of the GNU Affero General Public License, version 3, exempting you from
 * the requirement in Section 6 of the GNU General Public License, version 3,
 * to accompany Corresponding Source with Installation Information for the
 * Program or any work based on the Program. You are still required to
 * comply with all other Section 6 requirements to provide Corresponding
 * Source.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef _IBCS_US_LINUX26_COMPAT_H
#define _IBCS_US_LINUX26_COMPAT_H
#include <stdarg.h>
#include <stddef.h>

#include <ibcs-us/ibcs/ibcs-lib.h>
#include <ibcs-us/linux26-compat/linux/types.h>

#undef linux			/* I'm not sure where this comes from */


/*
 * Do a syscall to the emulated linux-2.6.32 user space API.
 */
#define	SYS(name, args...)	linux26_syscall(__NR_##name, ##args)
#define	SYS_NATIVE(name, args...) SYS(name, ##args)

struct file;

extern void linux26_compat_init(const char* me);
extern int linux26_fclose(struct file* file);
extern struct file* linux26_fopen(const char* path, int mode);
extern long long linux26_syscall(int syscall, ...);
extern int load_binfmt(const char* executable_path);
#endif
