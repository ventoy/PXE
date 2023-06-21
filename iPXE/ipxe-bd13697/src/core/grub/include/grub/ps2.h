/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2007,2008,2009  Free Software Foundation, Inc.
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

#ifndef GRUB_PS2_HEADER
#define GRUB_PS2_HEADER	1

#include <grub/types.h>

#define GRUB_AT_ACK                     0xfa
#define GRUB_AT_NACK                    0xfe
#define GRUB_AT_TRIES                   5

/* Make sure it's zeroed-out and set current_set at init.  */
struct grub_ps2_state
{
  int e0_received;
  int f0_received;
  grub_uint8_t led_status;
  short at_keyboard_status;
  grub_uint8_t current_set;
};

/* If there is a key pending, return it; otherwise return GRUB_TERM_NO_KEY.  */
int
grub_ps2_process_incoming_byte (struct grub_ps2_state *ps2_state,
				grub_uint8_t data);

#endif
