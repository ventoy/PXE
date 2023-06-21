/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2016  Free Software Foundation, Inc.
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

#ifndef GRUB_FDTBUS_HEADER
#define GRUB_FDTBUS_HEADER	1

#include <grub/fdt.h>
#include <grub/err.h>

struct grub_fdtbus_dev
{
  struct grub_fdtbus_dev *next;
  struct grub_fdtbus_dev *parent;
  int node;
  struct grub_fdtbus_driver *driver;
};

struct grub_fdtbus_driver
{
  struct grub_fdtbus_driver *next;
  struct grub_fdtbus_driver **prev;

  const char *compatible;

  grub_err_t (*attach) (const struct grub_fdtbus_dev *dev);
  void (*detach) (const struct grub_fdtbus_dev *dev);

  /* Message bus operations.  */
  grub_err_t (*send) (const struct grub_fdtbus_dev *dev, const void *data, grub_size_t sz);
  grub_err_t (*receive) (const struct grub_fdtbus_dev *dev, void *data, grub_size_t sz);
  grub_err_t (*start) (const struct grub_fdtbus_dev *dev);
  void (*stop) (const struct grub_fdtbus_dev *dev);
};

extern char EXPORT_VAR(grub_fdtbus_invalid_mapping)[1];

static inline int
grub_fdtbus_is_mapping_valid (volatile void *m)
{
  return m != grub_fdtbus_invalid_mapping;
}

volatile void *
EXPORT_FUNC(grub_fdtbus_map_reg) (const struct grub_fdtbus_dev *dev, int reg, grub_size_t *size);

const void *
EXPORT_FUNC(grub_fdtbus_get_fdt) (void);

const char *
EXPORT_FUNC(grub_fdtbus_get_name) (const struct grub_fdtbus_dev *dev);

const void *
EXPORT_FUNC(grub_fdtbus_get_prop) (const struct grub_fdtbus_dev *dev,
		      const char *name,
		      grub_uint32_t *len);

void
EXPORT_FUNC(grub_fdtbus_register) (struct grub_fdtbus_driver *driver);

void
EXPORT_FUNC(grub_fdtbus_unregister) (struct grub_fdtbus_driver *driver);

int
EXPORT_FUNC(grub_fdtbus_is_compatible) (const char *compat_string,
					const struct grub_fdtbus_dev *dev);

/* Must be called before any register(). */
/* dtb is assumed to be unfreeable and must remain
   valid for lifetime of GRUB.
 */
void
grub_fdtbus_init (const void *dtb, grub_size_t size);

#endif
