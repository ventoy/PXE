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

#if 0
void *
grub_memset (void *s, int c, grub_size_t len)
{
  grub_uint8_t *ptr;
  for (ptr = s; len; ptr++, len--)
    *ptr = c;
  return s;
}

void *
grub_memmove (void *dest, const void *src, grub_size_t n)
{
  char *d = (char *) dest;
  const char *s = (const char *) src;

  if (d < s)
    while (n--)
      *d++ = *s++;
  else
    {
      d += n;
      s += n;

      while (n--)
	*--d = *--s;
    }

  return dest;
}

int
grub_memcmp (const void *s1, const void *s2, grub_size_t n)
{
  const unsigned char *t1 = s1;
  const unsigned char *t2 = s2;

  while (n--)
    {
      if (*t1 != *t2)
	return (int) *t1 - (int) *t2;

      t1++;
      t2++;
    }

  return 0;
}
#endif /* #if 0 */

void *grub_decompressor_scratch;

void
find_scratch (void *src, void *dst, unsigned long srcsize,
	      unsigned long dstsize)
{
#ifdef _mips
  /* Decoding from ROM.  */
  if (((grub_addr_t) src & 0x10000000))
    {
      grub_decompressor_scratch = (void *) ALIGN_UP((grub_addr_t) dst + dstsize,
						    256);
      return;
    }
#endif
  if ((char *) src + srcsize > (char *) dst + dstsize)
    grub_decompressor_scratch = (void *) ALIGN_UP ((grub_addr_t) src + srcsize,
						   256);
  else
    grub_decompressor_scratch = (void *) ALIGN_UP ((grub_addr_t) dst + dstsize,
						   256);
  return;
}
