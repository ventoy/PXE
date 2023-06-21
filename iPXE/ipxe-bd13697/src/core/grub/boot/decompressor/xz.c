/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2010  Free Software Foundation, Inc.
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

#include <grub/types.h>
#include <grub/misc.h>
#include <grub/decompressor.h>

#include "xz.h"
#include "xz_stream.h"

void
grub_decompress_core (void *src, void *dst, unsigned long srcsize,
		      unsigned long dstsize)
{
  struct xz_dec *dec;
  struct xz_buf buf;

  find_scratch (src, dst, srcsize, dstsize);

  dec = xz_dec_init (GRUB_DECOMPRESSOR_DICT_SIZE);

  buf.in = src;
  buf.in_pos = 0;
  buf.in_size = srcsize;
  buf.out = dst;
  buf.out_pos = 0;
  buf.out_size = dstsize;

  while (buf.in_pos != buf.in_size)
    {
      enum xz_ret xzret;
      xzret = xz_dec_run (dec, &buf);
      switch (xzret)
	{
	case XZ_MEMLIMIT_ERROR:
	case XZ_FORMAT_ERROR:
	case XZ_OPTIONS_ERROR:
	case XZ_DATA_ERROR:
	case XZ_BUF_ERROR:
	  return;
	default:
	  break;
	}
    }
}
