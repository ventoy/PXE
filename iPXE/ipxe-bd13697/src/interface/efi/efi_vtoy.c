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

INTN StrCmp (
  CONST CHAR16              *FirstString,
  CONST CHAR16              *SecondString
  )
{
  while ((*FirstString != L'\0') && (*FirstString == *SecondString)) {
    FirstString++;
    SecondString++;
  }
  return *FirstString - *SecondString;
}

INTN StrAsciiCmp(CONST CHAR16 *FirstString, CONST char *SecondString)
{
    while ((*FirstString != L'\0') && (*FirstString == (CHAR16)(*SecondString))) {
        FirstString++;
        SecondString++;
    }
  return *FirstString - (CHAR16)(*SecondString);
}


CHAR16 * StrStr 
(
    CONST CHAR16              *String,
    CONST CHAR16              *SearchString
)
{
  CONST CHAR16 *FirstMatch;
  CONST CHAR16 *SearchStringTmp;

  if (*SearchString == L'\0') {
    return (CHAR16 *) String;
  }

  while (*String != L'\0') {
    SearchStringTmp = SearchString;
    FirstMatch = String;

    while ((*String == *SearchStringTmp)
            && (*String != L'\0')) {
      String++;
      SearchStringTmp++;
    }

    if (*SearchStringTmp == L'\0') {
      return (CHAR16 *) FirstMatch;
    }

    if (*String == L'\0') {
      return NULL;
    }

    String = FirstMatch + 1;
  }

  return NULL;
}

INTN vtoy_replace_match(CONST CHAR16 *FirstString, CONST char *SecondString)
{
    if (*FirstString == L'\\')
    {
        FirstString++;
    }
    
    if (*SecondString == '\\')
    {
        SecondString++;
    }

    return StrAsciiCmp(FirstString, SecondString);
}


EFI_STATUS vtoy_connect_driver(EFI_HANDLE ControllerHandle, const CHAR16 *DrvName)
{
    UINTN i = 0;
    UINTN Count = 0;
    CHAR16 *DriverName = NULL;
    EFI_HANDLE *Handles = NULL;
    EFI_HANDLE DrvHandles[2] = { NULL };
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_COMPONENT_NAME_PROTOCOL *NameProtocol = NULL;
    EFI_COMPONENT_NAME2_PROTOCOL *Name2Protocol = NULL;
    EFI_BOOT_SERVICES *gBS = efi_systab->BootServices;

    vdebug("ventoy_connect_driver <%ls>...\n", DrvName);

    Status = gBS->LocateHandleBuffer(ByProtocol, &efi_component_name2_protocol_guid, 
                                     NULL, &Count, &Handles);
    if (Status)
    {
        return Status;
    }

    for (i = 0; i < Count; i++)
    {
        Status = gBS->HandleProtocol(Handles[i], &efi_component_name2_protocol_guid, (VOID **)&Name2Protocol);
        if (Status)
        {
            continue;
        }

        VTOY_GET_COMPONENT_NAME(Name2Protocol, DriverName);

        if (StrStr(DriverName, DrvName))
        {
            vdebug("Find driver name2:<%ls>: <%ls>\n", DriverName, DrvName);
            DrvHandles[0] = Handles[i];
            break;
        }
    }

    if (i < Count)
    {
        Status = gBS->ConnectController(ControllerHandle, DrvHandles, NULL, TRUE);
        vdebug("ventoy_connect_driver:<%ls> <%s>\n", DrvName, efi_status(Status));
        goto end;
    }

    vdebug("%ls NOT found, now try COMPONENT_NAME\n", DrvName);

    Count = 0;
    gBS->FreePool(Handles);
    Handles = NULL;

    Status = gBS->LocateHandleBuffer(ByProtocol, &efi_component_name_protocol_guid, 
                                     NULL, &Count, &Handles);
    if (Status)
    {
        return Status;
    }

    for (i = 0; i < Count; i++)
    {
        Status = gBS->HandleProtocol(Handles[i], &efi_component_name_protocol_guid, (VOID **)&NameProtocol);
        if (Status)
        {
            continue;
        }

        VTOY_GET_COMPONENT_NAME(NameProtocol, DriverName);


        if (StrStr(DriverName, DrvName))
        {
            vdebug("Find driver name:<%ls>: <%ls>\n", DriverName, DrvName);
            DrvHandles[0] = Handles[i];
            break;
        }
    }

    if (i < Count)
    {
        Status = gBS->ConnectController(ControllerHandle, DrvHandles, NULL, TRUE);
        vdebug("ventoy_connect_driver:<%ls> <%s>\n", DrvName, efi_status(Status));
        goto end;
    }

    Status = EFI_NOT_FOUND;
    
end:
    gBS->FreePool(Handles);

    return Status;
}

