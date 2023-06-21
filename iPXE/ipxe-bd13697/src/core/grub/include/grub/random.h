/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2016 Free Software Foundation, Inc.
 *
 *  GRUB is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  GRUB is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GRUB.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef GRUB_RANDOM_HEADER
#define GRUB_RANDOM_HEADER 1

#include <grub/types.h>
#include <grub/err.h>

/* Not peer-reviewed. May not be any better than string of zeros.  */
grub_err_t
grub_crypto_get_random (void *buffer, grub_size_t sz);

/* Do not use directly. Use grub_crypto_get_random instead.  */
int
grub_crypto_arch_get_random (void *buffer, grub_size_t sz);

#endif
