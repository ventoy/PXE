/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2008,2009  Free Software Foundation, Inc.
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

#ifndef	GRUB_DMA_H
#define	GRUB_DMA_H	1

struct grub_pci_dma_chunk;

struct grub_pci_dma_chunk *EXPORT_FUNC(grub_memalign_dma32) (grub_size_t align,
							     grub_size_t size);
void EXPORT_FUNC(grub_dma_free) (struct grub_pci_dma_chunk *ch);
volatile void *EXPORT_FUNC(grub_dma_get_virt) (struct grub_pci_dma_chunk *ch);
grub_uint32_t EXPORT_FUNC(grub_dma_get_phys) (struct grub_pci_dma_chunk *ch);

static inline void *
grub_dma_phys2virt (grub_uint32_t phys, struct grub_pci_dma_chunk *chunk)
{
  return ((grub_uint8_t *) grub_dma_get_virt (chunk)
	  + (phys - grub_dma_get_phys (chunk)));
}

static inline grub_uint32_t
grub_dma_virt2phys (volatile void *virt, struct grub_pci_dma_chunk *chunk)
{
  return (((grub_uint8_t *) virt - (grub_uint8_t *) grub_dma_get_virt (chunk))
	  + grub_dma_get_phys (chunk));
}

#endif
