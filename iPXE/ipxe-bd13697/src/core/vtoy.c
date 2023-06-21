
FILE_LICENCE ( GPL2_OR_LATER_OR_UBDL );

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ipxe/deflate.h"
#include "ipxe/xferbuf.h"
#include "ipxe/umalloc.h"
#include <ipxe/image.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <ipxe/shell.h>
#include <usr/imgmgmt.h>
#include <ipxe/netdevice.h>
#include <ipxe/command.h>
#include <ipxe/parseopt.h>
#include <ipxe/in.h>
#include <ipxe/md5.h>
#include <ipxe/crypto.h>
#include <ipxe/vtoy.h>
#include <usr/ifmgmt.h>
#include <hci/ifmgmt_cmd.h>
#include <ctype.h>

#include <grub/misc.h>
#include <grub/err.h>
#include <grub/mm.h>
#include <stdarg.h>
#include <grub/term.h>
#include <grub/env.h>
#include <grub/i18n.h>
#include <grub/types.h>
#include <grub/fshelp.h>
#include <grub/dl.h>
#include <grub/file.h>
#include <grub/extcmd.h>

#define ZPAD 0x10

int g_timeout_flag = 0;

int g_debug_flag = 0;
int g_password_flag = 0;
static char g_szConsoleCmd[256];


int vtoy_calc_md5(const void *pdata, int len, char *md5str)
{
    int i;
    grub_uint8_t ctx[MD5_CTX_SIZE];
    grub_uint8_t result[MD5_DIGEST_SIZE];

    digest_init(&md5_algorithm, ctx);
    digest_update(&md5_algorithm, ctx, pdata, len);
    digest_final(&md5_algorithm, ctx, result);

    for (i = 0; i < (int)MD5_DIGEST_SIZE; i++)
    {
        snprintf(md5str + i * 2, 4, "%02x", result[i]);
    }

    return 0;
}

unsigned int vtoy_parse_ip(const char *szIP)
{
    char *p = (char *)szIP;
    unsigned int a[4];

    a[0] = (unsigned int)strtoul(p,   &p, 10);
    a[1] = (unsigned int)strtoul(p+1, &p, 10);
    a[2] = (unsigned int)strtoul(p+1, &p, 10);
    a[3] = (unsigned int)strtoul(p+1, &p, 10);

    return ((a[0] << 24) | (a[1] << 16) | (a[2] << 8) | a[3]);
}

char * vtoy_format_unsigned_decimal(char *end, unsigned long long num, int width, int flags) 
{
	char *ptr = end;
	int zpad = ( flags & ZPAD );
	int pad = ( zpad | ' ' );

	do {
		*(--ptr) = '0' + ( num % 10 );
		num /= 10;
	} while ( num );

	/* Pad to width */
	while ( ( end - ptr ) < width )
		*(--ptr) = pad;

	return ptr;
}


int vtoy_is_bootfilename_valid(const char *szBootFileName, const char *szServerId)
{
    const char *pcPrefix = "01-";

    if (NULL == szBootFileName)
    {
        return 0;
    }

    if (strlen(szBootFileName) <= 0)
    {
        printf("\nNo iVentoy boot file name offered from %s\n", szServerId);
        return 0;
    }
    else if (NULL == strstr(szBootFileName, pcPrefix))
    {
        printf("Invalid iVentoy boot file name format:%s from %s\n", 
               szBootFileName, szServerId);
        return 0;
    }
    else
    {
        return 1;
    }
}

int toy_store_setting(char *szKey, const char *szValue)
{
    struct named_setting setting;

    if (parse_autovivified_setting(szKey, &setting) != 0)
    {
        printf("parse_autovivified_setting %s failed", szKey);
        return -1;
    }

    setting.setting.type = &setting_type_string;    
    
    if (storef_setting(setting.settings, &setting.setting, szValue) != 0)
    {
        printf("storef_setting %s failed", szKey);
        return -1;
    }

    return 0;
}

int toy_setting_string_get(char *szKey, char *szValue, int iValBufLen)
{
    struct named_setting setting;

    if (parse_autovivified_setting(szKey, &setting) != 0)
    {
        printf("parse_autovivified_setting %s failed", szKey);
        return -1;
    }

    *szValue = 0;
    setting.setting.type = &setting_type_string;   

    return fetch_string_setting(setting.settings, &setting.setting, szValue, iValBufLen);
}

int toy_setting_hex_get(char *szKey, uint8_t *aucValue, int iValBufLen)
{
    struct named_setting setting;

    if (parse_autovivified_setting(szKey, &setting) != 0)
    {
        printf("parse_autovivified_setting %s failed", szKey);
        return -1;
    }

    setting.setting.type = &setting_type_hex;   

    return fetch_raw_setting(setting.settings, &setting.setting, aucValue, iValBufLen);
}

int toy_setting_uuid_get(char *szKey, uint8_t *aucValue, int iValBufLen)
{
    struct named_setting setting;

    if (parse_autovivified_setting(szKey, &setting) != 0)
    {
        printf("parse_autovivified_setting %s failed", szKey);
        return -1;
    }

    setting.setting.type = &setting_type_uuid;   

    return fetch_raw_setting(setting.settings, &setting.setting, aucValue, iValBufLen);
}


int vtoy_parse_server_http_port(const char *filename)
{
    int port;
    const char *pos;
    char szPort[16];

    if (strncmp(filename, "http://", 7) == 0)
    {
        pos = strchr(filename + 7, ':');
        if (pos)
        {
            port = (int)strtoul(pos + 1, NULL, 10);

            snprintf(szPort, sizeof(szPort), "%d", port);    
            toy_store_setting("server_http_port", szPort);        
            return 0;
        }
    }

    return -1;
}

int grub_file_exist(const char * fmt, ...)
{
    va_list ap;
    grub_file_t file;
    char fullpath[512] = {0};

    va_start (ap, fmt);
    grub_vsnprintf(fullpath, 511, fmt, ap);
    va_end (ap);

    file = grub_file_open(fullpath, GRUB_FILE_TYPE_NONE);
    if (!file)
    {
        grub_errno = 0;
        return 0;
    }
    else
    {
        grub_file_close(file);
        return 1;
    }
}

int toy_dhcp_discover_extlen(void)
{
    return (int)sizeof(DEV_INFO_S);
}

void toy_dhcp_discover_fill_ext(void *pBuf)
{
    DEV_INFO_S *pstDev = pBuf;

    pstDev->ucCode = 236;
    pstDev->ucLen = sizeof(DEV_INFO_S) - 2;

    toy_setting_string_get("manufacturer", pstDev->szManu, (int)sizeof(pstDev->szManu));
    toy_setting_string_get("product", pstDev->szProductName, (int)sizeof(pstDev->szProductName));
    toy_setting_string_get("serial", pstDev->szSerialNumber, (int)sizeof(pstDev->szSerialNumber));
    toy_setting_string_get("asset", pstDev->szAssetTag, (int)sizeof(pstDev->szAssetTag));
    toy_setting_uuid_get("uuid", pstDev->uuid, (int)sizeof(pstDev->uuid));
}


int vtoy_console(int argc, char * * argv)
{
    int i;

    strncpy(g_szConsoleCmd, "console", sizeof(g_szConsoleCmd));
    for (i = 1; i < argc; i++)
    {
        strcat(g_szConsoleCmd, " ");
        strcat(g_szConsoleCmd, argv[i]);
    }

    if (argc == 1)
    {
        strcat(g_szConsoleCmd, " -x 1024 -y 768");
    }

    system(g_szConsoleCmd);
    return 0;
}

int vtoy_last_console(int argc, char * * argv)
{
    (void)argc;
    (void)argv;

    if (g_szConsoleCmd[0]) {
        system(g_szConsoleCmd);
    }
    return 0;
}

void vtoy_init(void)
{
    #if (TOY_UEFI == 1)
    toy_store_setting("TOY_ARCH", TOY_ARCH " UEFI");
    #else
    toy_store_setting("TOY_ARCH", TOY_ARCH " BIOS");
    #endif

    //system("colour --rgb 0x505050 3");
    system("cpair -f 7 -b 6 2");

    grub_mod_crc64_init();
    
    grub_sandisk_init();    
    grub_mod_loopback_init();    
    grub_mod_squashfs_init();
    grub_mod_ext2_init();
    grub_mod_iso9660_init();   
}

