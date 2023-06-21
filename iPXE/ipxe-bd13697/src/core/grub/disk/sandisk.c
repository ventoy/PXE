/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2006,2007,2008  Free Software Foundation, Inc.
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ipxe/vtoy.h>
#include <ipxe/sanboot.h>

#include <grub/disk.h>
#include <grub/partition.h>
#include <grub/mm.h>
#include <grub/types.h>
#include <grub/misc.h>
#include <grub/err.h>
#include <grub/term.h>

static int
grub_sandisk_iterate (grub_disk_dev_iterate_hook_t hook, void *hook_data,
		      grub_disk_pull_t pull)
{
    char buf[16];

    if (pull == GRUB_DISK_PULL_NONE)
    {
        grub_snprintf (buf, sizeof (buf), "hd0");
        grub_dprintf ("sandisk", "iterating %s\n", buf);
        if (hook (buf, hook_data))
            return 1;
    }
    
    return 0;
}

static grub_err_t grub_sandisk_open (const char *name, struct grub_disk *disk)
{
    struct san_device *sandev = NULL;
    
    grub_dprintf ("sandisk", "opening %s\n", name);

    if (!name || strcmp(name, "hd0"))
    {
        return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such device");
    }

    sandev = sandev_first();
    if (!sandev)
    {
        return grub_error (GRUB_ERR_UNKNOWN_DEVICE, "no such device");
    }

    disk->data = sandev;
    disk->total_sectors = sandev_capacity(sandev);
    disk->log_sector_size = 11;

    grub_dprintf ("sandisk", "opening %s succeeded\n", name);

    return GRUB_ERR_NONE;
}

static void grub_sandisk_close (struct grub_disk *disk __attribute__ ((unused)))
{
    grub_dprintf ("sandisk", "closing %s\n", disk->name);
}

static grub_err_t
grub_sandisk_read (struct grub_disk *disk, grub_disk_addr_t sector, grub_size_t size, char *buf)
{
    return sandev_read(disk->data, sector, size, (userptr_t)buf);
}

static grub_err_t
grub_sandisk_write (struct grub_disk *disk, grub_disk_addr_t sector,
		    grub_size_t size, const char *buf)
{
    (void)disk;
    (void)sector;
    (void)size;
    (void)buf;

    return GRUB_ERR_ACCESS_DENIED;
}

static struct grub_disk_dev grub_sandisk_dev =
{
    .name = "sandisk",
    .id = GRUB_DISK_DEVICE_SAN_ID,
    .disk_iterate = grub_sandisk_iterate,
    .disk_open = grub_sandisk_open,
    .disk_close = grub_sandisk_close,
    .disk_read = grub_sandisk_read,
    .disk_write = grub_sandisk_write,
    .next = 0
};


void grub_sandisk_init (void)
{
    grub_disk_dev_register (&grub_sandisk_dev);
}

