#ifndef _IPXE_VTOY_H
#define _IPXE_VTOY_H

#include <stdio.h>

/** @file
 * vtoy define
 */

FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );


#define DIR_STATUS_LIB_EXIST        1
#define DIR_STATUS_MODULES_EXIST    2
#define DIR_STATUS_USR_EXIST        4
#define DIR_STATUS_NIXOS_LAYOUT     8


#define MAX_KV_IN_SFS   64

typedef struct SFS_KV
{
    int kvs;
    char aszKv[MAX_KV_IN_SFS][64];
}SFS_KV_S;

typedef struct netdrv_node
{
    struct netdrv_node *pstNext;
    char szKoPath[256];
    int KoSize;
}netdrv_node;

#pragma pack(1)

typedef struct netdrv_data_node
{
    uint32_t uiFileLen;
    char szKoPath[256];
    /* uiFileLen file content data */
}netdrv_data_node;

#pragma pack()

#define check_goto_end(p, fmt, ...) \
    if (!p) \
    {\
        vdebug(fmt, ##__VA_ARGS__);\
        goto end;\
    }

#define grub_check_free(p) \
{\
    if (p) { \
        grub_free(p); \
        p = NULL;\
    }\
}

#define grub_file_check_close(f) \
{\
    if (f) { \
        grub_file_close(f); \
        f = NULL;\
    }\
}

#if (TOY_BIOS == 1)
#define ventoy_debug_pause()    \
if (g_debug_flag) \
{\
    printf("\nPress Ctrl+C to continue......");\
    sleep(3600);\
    printf("\n");\
}

#else
#define ventoy_debug_pause() \
if (g_debug_flag) \
{ \
    UINTN __Index = 0; \
    efi_systab->ConOut->OutputString(efi_systab->ConOut, L"[VTOY] ###### Press Enter to continue... ######\r\n");\
    efi_systab->ConIn->Reset(efi_systab->ConIn, FALSE); \
    efi_systab->BootServices->WaitForEvent(1, &efi_systab->ConIn->WaitForKey, &__Index);\
}

#define ventoy_pause() \
{ \
    UINTN __Index = 0; \
    efi_systab->ConOut->OutputString(efi_systab->ConOut, L"[VTOY] ###### Press Enter to continue... ######\r\n");\
    efi_systab->BootServices->WaitForEvent(1, &efi_systab->ConIn->WaitForKey, &__Index);\
}

#endif


#pragma pack(1)
typedef struct DEV_INFO
{
    uint8_t ucCode;   /* 固定236 */
    uint8_t ucLen;    /* 长度: 固定为    178 */
    char szManu[32];
    char szProductName[64];
    char szSerialNumber[32];
    char szAssetTag[32];
    uint8_t uuid[16];
}DEV_INFO_S;
#pragma pack()



extern int g_debug_flag;
extern int g_password_flag;
extern int g_timeout_flag;

extern char *g_pcSandiskOverBuf;
extern uint32_t g_uiSandiskOverLen;
extern int g_aiSandiskOverCnt;
extern uint64_t g_aui64SandiskOverLoc[16];


extern char g_szEFIReplace[128];
extern uint32_t g_uiReplaceSector;
extern uint32_t g_uiReplaceSize;



#define vdebug(fmt, ...) if (g_debug_flag) dbg_printf(fmt, ##__VA_ARGS__)

#define argshift()  argv++; argc--

void vtoy_init(void);
int vtoy_parse_server_http_port(const char *filename);
int toy_store_setting(char *szKey, const char *szValue);
int toy_setting_string_get(char *szKey, char *szValue, int iValBufLen);
int toy_setting_hex_get(char *szKey, uint8_t *aucValue, int iValBufLen);
int toy_setting_uuid_get(char *szKey, uint8_t *aucValue, int iValBufLen);
int vtoy_is_bootfilename_valid(const char *szBootFileName, const char *szServerId);
char * vtoy_format_unsigned_decimal(char *end, unsigned long long num, int width, int flags);
unsigned int vtoy_parse_ip(const char *szIP);

void grub_mod_iso9660_init(void);
void grub_mod_ext2_init(void);
void grub_mod_squashfs_init(void);
void grub_sandisk_init (void);
void grub_mod_loopback_init(void);
void grub_mod_crc64_init(void);
int grub_file_exist(const char * fmt, ...);

int toy_dhcp_discover_extlen(void);
void toy_dhcp_discover_fill_ext(void *pBuf);
int vtoy_console(int argc, char * * argv);
int vtoy_last_console(int argc, char * * argv);
int vtoy_calc_md5(const void *pdata, int len, char *md5str);

#endif /* _IPXE_VTOY_H */

