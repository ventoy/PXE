#ifndef _IPXE_EFI_VTOY_H
#define _IPXE_EFI_VTOY_H

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );


/** EFI SAN device private data */
struct efi_block_data {
	/** SAN device */
	struct san_device *sandev;
	/** EFI handle */
	EFI_HANDLE handle;
	/** Media descriptor */
	EFI_BLOCK_IO_MEDIA media;
	/** Block I/O protocol */
	EFI_BLOCK_IO_PROTOCOL block_io;
	/** Device path protocol */
	EFI_DEVICE_PATH_PROTOCOL *path;
};


#define VTOY_GET_COMPONENT_NAME(Protocol, DriverName) \
{\
    DriverName = NULL;\
    Status = Protocol->GetDriverName(Protocol, "en", &DriverName);\
    if (Status || NULL == DriverName) \
    {\
        DriverName = NULL;\
        Status = Protocol->GetDriverName(Protocol, "eng", &DriverName);\
        if (Status || NULL == DriverName) \
        {\
            continue;\
        }\
    }\
}

EFI_STATUS vtoy_connect_driver(EFI_HANDLE ControllerHandle, const CHAR16 *DrvName);


EFI_STATUS EFIAPI ventoy_wrapper_open_volume
(
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL     *This,
    EFI_FILE_PROTOCOL                 **Root
);

INTN vtoy_replace_match(CONST CHAR16 *FirstString, CONST char *SecondString);
EFI_STATUS ventoy_wrapper_push_openvolume(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_OPEN_VOLUME OpenVolume);
INTN StrAsciiCmp(CONST CHAR16 *FirstString, CONST char *SecondString);
INTN StrCmp (
  CONST CHAR16              *FirstString,
  CONST CHAR16              *SecondString
  );

#endif /* _IPXE_EFI_VTOY_H */

