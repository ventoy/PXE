/*
 * Copyright (C) 2016 Michael Brown <mbrown@fensystems.co.uk>.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * You can also choose to distribute this program under the terms of
 * the Unmodified Binary Distribution Licence (as given in the file
 * COPYING.UBDL), provided that you have satisfied its requirements.
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

/**
 * @file
 *
 * EFI block device protocols
 *
 */

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ipxe/refcnt.h>
#include <ipxe/list.h>
#include <ipxe/uri.h>
#include <ipxe/interface.h>
#include <ipxe/blockdev.h>
#include <ipxe/xfer.h>
#include <ipxe/open.h>
#include <ipxe/retry.h>
#include <ipxe/timer.h>
#include <ipxe/process.h>
#include <ipxe/sanboot.h>
#include <ipxe/iso9660.h>
#include <ipxe/acpi.h>
#include <ipxe/efi/efi.h>
#include <ipxe/efi/Protocol/BlockIo.h>
#include <ipxe/efi/Protocol/SimpleFileSystem.h>
#include <ipxe/efi/Protocol/AcpiTable.h>
#include <ipxe/efi/Guid/FileInfo.h>
#include <ipxe/efi/efi_driver.h>
#include <ipxe/efi/efi_strings.h>
#include <ipxe/efi/efi_snp.h>
#include <ipxe/efi/efi_path.h>
#include <ipxe/efi/efi_null.h>
#include <ipxe/efi/efi_block.h>

#include <ipxe/vtoy.h>
#include <ipxe/efi/efi_vtoy.h>
#include <ipxe/efi/efi_wrap.h>
#include <ipxe/efi/Protocol/ComponentName.h>
#include <ipxe/efi/Protocol/ComponentName2.h>


static EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME g_original_open_volume;
static EFI_FILE_OPEN g_original_fopen;
static EFI_FILE_CLOSE g_original_fclose;
static EFI_FILE_PROTOCOL  g_wrapper_handle;
static UINT64 g_replace_cur_pos = 0;
static UINT8 g_aucSectorBuf[2048];


STATIC EFI_STATUS EFIAPI
ventoy_wrapper_fs_open(EFI_FILE_HANDLE This, EFI_FILE_HANDLE *New, CHAR16 *Name, UINT64 Mode, UINT64 Attributes)
{
    (VOID)This;
    (VOID)New;
    (VOID)Name;
    (VOID)Mode;
    (VOID)Attributes;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_open_ex(EFI_FILE_HANDLE This, EFI_FILE_HANDLE *New, CHAR16 *Name, UINT64 Mode, UINT64 Attributes, EFI_FILE_IO_TOKEN *Token)
{
    (void)Token;
	return ventoy_wrapper_fs_open(This, New, Name, Mode, Attributes);
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_delete(EFI_FILE_HANDLE This)
{
    (VOID)This;
	return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_set_info(EFI_FILE_HANDLE This, EFI_GUID *Type, UINTN Len, VOID *Data)
{
    (void)This;
    (void)Type;
    (void)Len;
    (void)Data;
	return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_flush(EFI_FILE_HANDLE This)
{
    (VOID)This;
	return EFI_SUCCESS;
}

/* Ex version */
STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_flush_ex(EFI_FILE_HANDLE This, EFI_FILE_IO_TOKEN *Token)
{
    (VOID)This;
    (VOID)Token;
	return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_write(EFI_FILE_HANDLE This, UINTN *Len, VOID *Data)
{
    (VOID)This;
    (VOID)Len;
    (VOID)Data;

	return EFI_WRITE_PROTECTED;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_write_ex(IN EFI_FILE_PROTOCOL *This, IN OUT EFI_FILE_IO_TOKEN *Token)
{
    (VOID)Token;
	return ventoy_wrapper_file_write(This, &(Token->BufferSize), Token->Buffer);
}


STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_close(EFI_FILE_HANDLE This)
{
    (VOID)This;
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_set_pos(EFI_FILE_HANDLE This, UINT64 Position)
{
    (VOID)This;
    
    if (Position <= (UINT64)g_uiReplaceSize)
    {
        g_replace_cur_pos = Position;
    }
    else
    {
        g_replace_cur_pos = g_uiReplaceSize;
    }
    
    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_get_pos(EFI_FILE_HANDLE This, UINT64 *Position)
{
    (VOID)This;
    
    *Position = g_replace_cur_pos;
    return EFI_SUCCESS;
}


STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_get_info(EFI_FILE_HANDLE This, EFI_GUID *Type, UINTN *Len, VOID *Data)
{
    EFI_FILE_INFO *Info = (EFI_FILE_INFO *) Data;

    (VOID)This;
    (VOID)Type;

    if (*Len == 0)
    {
        *Len = 384;
        return EFI_BUFFER_TOO_SMALL;
    }

    memset(Data, 0, sizeof(EFI_FILE_INFO));

    Info->Size = sizeof(EFI_FILE_INFO);
    Info->FileSize = g_uiReplaceSize;
    Info->PhysicalSize = g_uiReplaceSize;
    Info->Attribute = EFI_FILE_READ_ONLY;
    //Info->FileName = EFI_FILE_READ_ONLY;

    *Len = Info->Size;
    
    return EFI_SUCCESS;
}


STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_read(EFI_FILE_HANDLE This, UINTN *Len, VOID *Data)
{
    EFI_LBA Lba;
    UINTN PartLen = 0;
    UINTN ReadSec = 0;
    UINTN ReadLen = *Len;
    struct efi_block_data *block = NULL;
    struct san_device *sandev = NULL;
    
    (VOID)This;

    if (g_replace_cur_pos + ReadLen > (UINT64)g_uiReplaceSize)
    {
        ReadLen = g_uiReplaceSize - g_replace_cur_pos;
    }

    Lba = g_replace_cur_pos / 2048 + g_uiReplaceSector;

    sandev = sandev_first();
    block = (struct efi_block_data *)sandev->priv;

    PartLen = ReadLen % 2048;
    ReadSec = (ReadLen - PartLen) / 2048;

    block->block_io.ReadBlocks(&block->block_io, block->media.MediaId, Lba, ReadLen - PartLen, Data);

    if (PartLen > 0)
    {
        block->block_io.ReadBlocks(&block->block_io, block->media.MediaId, Lba + ReadSec, 2048, g_aucSectorBuf);
        memcpy((char *)Data + ReadLen - PartLen, g_aucSectorBuf, PartLen);
    }
    
    *Len = ReadLen;
    g_replace_cur_pos += ReadLen;

    return EFI_SUCCESS;
}

STATIC EFI_STATUS EFIAPI
ventoy_wrapper_file_read_ex(IN EFI_FILE_PROTOCOL *This, IN OUT EFI_FILE_IO_TOKEN *Token)
{
	return ventoy_wrapper_file_read(This, &(Token->BufferSize), Token->Buffer);
}

EFI_STATUS EFIAPI ventoy_wrapper_file_procotol(EFI_FILE_PROTOCOL *File)
{
    File->Revision    = EFI_FILE_PROTOCOL_REVISION2;
    File->Open        = ventoy_wrapper_fs_open;
    File->Close       = ventoy_wrapper_file_close;
    File->Delete      = ventoy_wrapper_file_delete;
    File->Read        = ventoy_wrapper_file_read;
    File->Write       = ventoy_wrapper_file_write;
    File->GetPosition = ventoy_wrapper_file_get_pos;
    File->SetPosition = ventoy_wrapper_file_set_pos;
    File->GetInfo     = ventoy_wrapper_file_get_info;
    File->SetInfo     = ventoy_wrapper_file_set_info;
    File->Flush       = ventoy_wrapper_file_flush;
    File->OpenEx      = ventoy_wrapper_file_open_ex;
    File->ReadEx      = ventoy_wrapper_file_read_ex;
    File->WriteEx     = ventoy_wrapper_file_write_ex;
    File->FlushEx     = ventoy_wrapper_file_flush_ex;

    return EFI_SUCCESS;
}

EFIAPI EFI_STATUS ventoy_wrapper_file_open
(
    EFI_FILE_HANDLE This, 
    EFI_FILE_HANDLE *New,
    CHAR16 *Name, 
    UINT64 Mode, 
    UINT64 Attributes
)
{
    EFI_STATUS Status = EFI_SUCCESS;

    if ((Mode & EFI_FILE_MODE_WRITE) > 0 && StrCmp(Name, L"\\loader\\random-seed") == 0)
    {
        if (g_debug_flag)
        {
            vdebug("## ventoy_wrapper_file_open return NOT_FOUND for random-seed %lx\n", (long)Mode);
        }
        return EFI_NOT_FOUND;
    }

    Status = g_original_fopen(This, New, Name, Mode, Attributes);  
    if (Status)
    {
        return Status;
    }

    if (vtoy_replace_match(Name, g_szEFIReplace) == 0)
    {
        g_original_fclose(*New);
        *New = &g_wrapper_handle;
        ventoy_wrapper_file_procotol(*New);        
    }

    return Status;
}


EFI_STATUS ventoy_wrapper_push_openvolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume)
{
    g_original_open_volume = OpenVolume;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI ventoy_wrapper_open_volume
(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL     *This,
    EFI_FILE_PROTOCOL                 **Root
)
{
    EFI_STATUS Status = EFI_SUCCESS;

    Status = g_original_open_volume(This, Root);
    if (Status == EFI_SUCCESS)
    {
        g_original_fopen = (*Root)->Open;
        g_original_fclose = (*Root)->Close;
        (*Root)->Open = ventoy_wrapper_file_open;
    }

    return Status;
}


