
#include <stdio.h>
#include <stdlib.h>
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

#include "ipxe/umalloc.h"
#include <ipxe/image.h>
#include <ipxe/command.h>
#include <ipxe/vtoy.h>
#include <ipxe/md5.h>
#include <ipxe/sha1.h>
#include <ipxe/sha256.h>
#include <ipxe/timer.h>
#include <ipxe/dhcp.h>
#include <ipxe/if_ether.h>
#include <strings.h>
#include <readline/readline.h>

#if (TOY_BIOS == 0)
#include <ipxe/efi/efi.h>
#endif

#define FILE_BUF_SIZE       65536
#define VENTOY_FILE_TYPE    (GRUB_FILE_TYPE_NO_DECOMPRESS | GRUB_FILE_TYPE_LINUX_INITRD)
grub_err_t grub_cmd_ls (grub_extcmd_context_t ctxt, int argc, char **args);
grub_err_t grub_cmd_loopback (grub_extcmd_context_t ctxt, int argc, char **args);
grub_err_t grub_cmd_hashsum (struct grub_extcmd_context *ctxt, int argc, char **args);
grub_err_t grub_cmd_cat (grub_extcmd_context_t ctxt, int argc, char **args);
grub_err_t grub_cmd_hexdump (grub_extcmd_context_t ctxt, int argc, char **args);
grub_uint32_t grub_getcrc32c (grub_uint32_t crc, const void *buf, int size);
extern struct grub_fs grub_squash_fs;

struct grub_term_output *grub_term_outputs = 0;

static int g_iNetDrvCount = 0;
static int g_iNetDrvTotLen = 0;
static netdrv_node *g_pstNetDrvList = NULL;
static netdrv_node *g_pstNetDrvTail = NULL;

char *g_pcSandiskOverBuf = NULL;
uint32_t g_uiSandiskOverLen = 0;

int g_aiSandiskOverCnt = 0;
uint64_t g_aui64SandiskOverLoc[16] = { 0 };

static char g_szNixOsModule[256];

int g_iSfsDirCount = 0;
char g_szSfsLastDirName[128];


char g_szEFIReplace[128];
uint32_t g_uiReplaceSector = 0;
uint32_t g_uiReplaceSize = 0;


typedef void (*grub_xputs_pf) (const char *str);


int grub_getkey_noblock (void)
{
    return GRUB_TERM_NO_KEY;
}

void * grub_malloc(grub_size_t size)
{
    
    return (void *)umalloc(size);
}

void grub_free(void *p)
{
    ufree((userptr_t)p);
}

void * grub_zalloc(grub_size_t size)
{
    void *p = (void *)umalloc(size);

    if (p)
    {
        memset(p, 0, size);
    }

    return p;
}

void *
grub_realloc (void *ptr, grub_size_t size)
{
    return (void *)urealloc((userptr_t)ptr, size);
}

int
grub_getkey (void)
{
  return -1;
}

void grub_exit (void)
{
    
}

int
grub_dl_ref (grub_dl_t mod)
{
  (void) mod;
  return 0;
}

int
grub_dl_unref (grub_dl_t mod)
{
  (void) mod;
  return 0;
}

void grub_refresh (void)
{

}

void grub_xputs_wrapper(const char *str)
{
    printf("%s", str);
}

grub_xputs_pf grub_xputs = grub_xputs_wrapper;


static int grub_debug_exec(int argc, char **argv)
{
    (void)argc;

    if (argv[1] == NULL)
    {
        printf("Usage: vtdebug { on | off }\n");
        return 1;
    }

    if (strcmp(argv[1], "on") == 0)
    {
        g_debug_flag = 1;
    }
    else if (strcmp(argv[1], "off") == 0)
    {
        g_debug_flag = 0; 
    }
    else
    {
        printf("Usage: vtdebug { on | off }\n");
        return 1;
    }

    return 0;
}

static int grub_ls_exec(int argc, char **argv)
{
    struct grub_arg_list opts[8];
    struct grub_extcmd_context ctxt;

    grub_errno = 0;

    memset(&ctxt, 0, sizeof(ctxt));
    memset(opts, 0, sizeof(opts));

    ctxt.state = opts;

    argshift();

    if (argv[0] && strcmp(argv[0], "-l") == 0)
    {
        opts[0].set = 1;
        argshift();
    }
    
    if (argv[0] && strcmp(argv[0], "-h") == 0)
    {
        opts[1].set = 1;
        argshift();
    }
    
    if (argv[0] && strcmp(argv[0], "-a") == 0)
    {
        opts[2].set = 1;
        argshift();
    }

    grub_cmd_ls(&ctxt, argc, argv);

    return 0;
}

int grub_loopback_exec(int argc, char **argv)
{
    struct grub_arg_list opts[8];
    struct grub_extcmd_context ctxt;

    grub_errno = 0;
    memset(&ctxt, 0, sizeof(ctxt));
    memset(opts, 0, sizeof(opts));

    ctxt.state = opts;

    argshift();

    if (argv[0] && strcmp(argv[0], "-d") == 0)
    {
        opts[0].set = 1;
        argshift();
    }

    grub_cmd_loopback(&ctxt, argc, argv);

    return 0;
}

int grub_cat_exec(int argc, char **argv)
{
    struct grub_arg_list opts[8];
    struct grub_extcmd_context ctxt;

    grub_errno = 0;
    memset(&ctxt, 0, sizeof(ctxt));
    memset(opts, 0, sizeof(opts));

    ctxt.state = opts;
    argshift();

    grub_cmd_cat(&ctxt, argc, argv);

    return 0;
}

int grub_hashsum_exec(int argc, char **argv)
{
    struct grub_arg_list opts[8];
    struct grub_extcmd_context ctxt;
    struct grub_extcmd extcmd;
    struct grub_command cmd;

    grub_errno = 0;
    memset(&ctxt, 0, sizeof(ctxt));
    memset(opts, 0, sizeof(opts));
    memset(&extcmd, 0, sizeof(extcmd));
    memset(&cmd, 0, sizeof(cmd));

    ctxt.state = opts;
    ctxt.extcmd = &extcmd;
    extcmd.cmd = &cmd;

    cmd.name = argv[0];

    argshift();

    if (argv[0] && strcmp(argv[0], "-n") == 0)
    {
        opts[5].set = 1;
        argshift();

        opts[5].arg = argv[0];
        argshift();
    }

    grub_cmd_hashsum(&ctxt, argc, argv);

    return 0;
}


int grub_hexdump_exec(int argc, char **argv)
{
    struct grub_arg_list opts[8];
    struct grub_extcmd_context ctxt;

    grub_errno = 0;
    memset(&ctxt, 0, sizeof(ctxt));
    memset(opts, 0, sizeof(opts));

    ctxt.state = opts;

    argshift();

    if (argv[0] && strcmp(argv[0], "-s") == 0)
    {
        opts[0].set = 1;
        argshift();

        opts[0].arg = argv[0];
        argshift();
    }

    if (argv[0] && strcmp(argv[0], "-n") == 0)
    {
        opts[1].set = 1;
        argshift();

        opts[1].arg = argv[0];
        argshift();
    }

    grub_cmd_hexdump(&ctxt, argc, argv);

    return 0;
}

static void vtoy_notify_progress(int percent)
{
    char szBuf[256];

    if (g_debug_flag)
    {
        return;
    }

    if (percent == 100)
    {
        snprintf(szBuf, sizeof(szBuf), "\rPreparing for boot, please wait...  100%%\n");
    }
    else
    {
        snprintf(szBuf, sizeof(szBuf), "\rPreparing for boot, please wait...  %d%%", percent);        
    }

    printf ("\033[32m" "%s" "\033[0m", szBuf); 
}

static int vtoy_dir_sfs_enum(const char *filename, const struct grub_dirhook_info *info, void *data)
{
    if (info->dir)
    {
        if ((filename[0] == '.' && filename[1] == 0) || 
            (filename[0] == '.' && filename[1] == '.' && filename[2] == 0))
        {
            return 0;
        }
    
        g_iSfsDirCount++;
        grub_strncpy(g_szSfsLastDirName, filename, sizeof(g_szSfsLastDirName));
        
        if (strcmp(filename, "lib") == 0)
        {
            *(int *)data |= DIR_STATUS_LIB_EXIST;
        }
        else if (strcmp(filename, "modules") == 0)
        {
            *(int *)data |= DIR_STATUS_MODULES_EXIST;
        }
        else if (strcmp(filename, "usr") == 0)
        {
            *(int *)data |= DIR_STATUS_USR_EXIST;
        }
        else
        {
            int len = 0;
            const char *pos = NULL;
            
            len = (int)strlen(filename);
            if (len > 32 && strncmp(filename + 32, "-linux-", 7) == 0)
            {
                pos = filename + 39;
                while (*pos)
                {
                    if (grub_isdigit(*pos) || *pos == '.')
                    {
                        pos++;                        
                    }
                    else
                    {
                        break;
                    }
                }

                if (*pos == 0)
                {
                    *(int *)data |= DIR_STATUS_NIXOS_LAYOUT; 
                    grub_strncpy(g_szNixOsModule, filename, sizeof(g_szNixOsModule));
                }
            }
        }
    }

    return 0;
}

static int vtoy_sfs_enum(const char *filename, const struct grub_dirhook_info *info, void *data)
{
    SFS_KV_S *pstKvs = data;

    if (info->dir && (filename[0] >= '0' && filename[0] <= '9'))
    {
        if (pstKvs->kvs < MAX_KV_IN_SFS)
        {
            grub_snprintf(pstKvs->aszKv[pstKvs->kvs], 64, "%s", filename);
            pstKvs->kvs++;
        }    
    }

    return 0;
}


static void vtoy_get_ko_suffix(char *pcModepBuf, char *szSuffix)
{
    char *pcPos1 = NULL;
    char *pcPos2 = NULL;

    szSuffix[0] = 0;

    pcPos1 = strchr(pcModepBuf, ':');
    if (!pcPos1)
    {
        return;
    }

    pcPos2 = pcPos1;
    while (pcPos2 != pcModepBuf)
    {
        if (*pcPos2 == '.')
        {
            break;
        }
        pcPos2--;
    }

    if (pcPos2 == pcModepBuf)
    {
        return;
    }

    *pcPos1 = 0;
    snprintf(szSuffix, 8, "%s", pcPos2);
    if (strcmp(szSuffix, ".ko") == 0)
    {
        szSuffix[0] = 0;
    }
    *pcPos1 = ':';    
}

static void vtoy_split_line(char *pcBuf)
{
    char *pcEnd = NULL;
    char *pcStart = pcBuf;

    while ((*pcStart) && ((pcEnd = strchr(pcStart, '\n')) != NULL))
    {
        *pcEnd = 0;
        pcStart = pcEnd + 1;
    }
}

static char * vtoy_find_dep_line(char *pcModepBuf, char *szKo)
{
    int len;
    char *pcPos = NULL;
    char *pcEnd = NULL;
    char *pcStart = pcModepBuf;

    len = (int)strlen(szKo);
    
    while ((*pcStart) && ((pcEnd = strchr(pcStart, '\0')) != NULL))
    {
        pcPos = strchr(pcStart, ':');
        if (!pcPos)
        {
            continue;
        }

        if (strncmp(pcPos - len + 1, szKo, len) == 0)
        {
            return pcStart;
        }
        pcStart = pcEnd + 1;
    }

    return NULL;
}

static int vtoy_match_netdrv(char *pcModaliBuf, uint8_t *puiPciID, char *szKoPath, int buflen)
{
    int len = 0;
    int rc = 1;
    char *pcEnd = NULL;
    char *pcStart = pcModaliBuf;
    char szAlias[128];

    len = (int)snprintf(szAlias, sizeof(szAlias), 
        "alias pci:v0000%02X%02Xd0000%02X%02Xsv*sd*bc*sc*i* ", 
        puiPciID[0], puiPciID[1], puiPciID[2], puiPciID[3]);

    vdebug("Search for %s\n", szAlias);

    while ((*pcStart) && ((pcEnd = strchr(pcStart, '\0')) != NULL))
    {
        if (strncmp(pcStart, szAlias, len) == 0)
        {
            snprintf(szKoPath, buflen, "%s", pcStart + len);
            vdebug("Find driver for PCI:%02X%02X%02X%02X <%s>\n", 
                puiPciID[0], puiPciID[1], puiPciID[2], puiPciID[3], szKoPath);
            rc = 0;
            break;
        }
        pcStart = pcEnd + 1;
    }

    len = (int)snprintf(szAlias, sizeof(szAlias), "alias %s ", szKoPath);

    pcStart = pcModaliBuf;
    while ((*pcStart) && ((pcEnd = strchr(pcStart, '\0')) != NULL))
    {
        if (strncmp(pcStart, szAlias, len) == 0)
        {
            snprintf(szKoPath, buflen, "%s", pcStart + len);
            vdebug("Update driver alias to <%s>\n", szKoPath);
            break;
        }
        pcStart = pcEnd + 1;
    }

    return rc;
}

static netdrv_node * vtoy_find_netdrv(const char *szKoPath)
{
    netdrv_node *pstNode = NULL;

    for (pstNode = g_pstNetDrvList; pstNode; pstNode = pstNode->pstNext)
    {
        if (strcmp(szKoPath, pstNode->szKoPath) == 0)
        {
            return pstNode;
        }
    }

    return NULL;
}


static int vtoy_push_netdrv(char *pcModepBuf, char *szMod, char *szKv, char *szSearch)
{
    int len;
    char *pcPos = NULL;
    char *pcDep = NULL;
    char *pcLine = NULL;
    netdrv_node *pstNode = NULL;
    grub_file_t file = NULL;

    vdebug("push net drv <%s> <%s> <%s>\n", szMod, szKv, szSearch);
    
    pcLine = vtoy_find_dep_line(pcModepBuf, szSearch);
    if (!pcLine)
    {
        vdebug("dep line not found for <%s>, maybe built-in or not built.\n", szSearch);
        return 0;
    }

    pcLine = strdup(pcLine);
    len = (int)strlen(pcLine);

    pcDep = strchr(pcLine, ':');
    if (!pcDep)
    {
        free(pcLine);
        vdebug("Colon not found\n");
        return 0;
    }

    pcPos = pcLine + len - 1;
    while (pcPos != pcLine)
    {
        if (*pcPos == ' ')
        {
            pstNode = grub_zalloc(sizeof(netdrv_node));
            if (pstNode)
            {
                grub_snprintf(pstNode->szKoPath, sizeof(pstNode->szKoPath), "%s%s/%s", szMod, szKv, pcPos + 1);
                if (vtoy_find_netdrv(pstNode->szKoPath))
                {
                    grub_free(pstNode);
                }
                else
                {                    
                    file = grub_file_open(pstNode->szKoPath, VENTOY_FILE_TYPE);
                    if (file)
                    {
                        pstNode->KoSize = (int)file->size;
                        grub_file_close(file);

                        if (g_pstNetDrvList)
                        {
                            g_pstNetDrvTail->pstNext = pstNode;
                            g_pstNetDrvTail = pstNode;
                        }
                        else
                        {
                            g_pstNetDrvList = g_pstNetDrvTail = pstNode;
                        }

                        g_iNetDrvCount++;
                        g_iNetDrvTotLen += pstNode->KoSize;                    
                    }
                    else
                    {
                        grub_free(pstNode);
                    }
                }                
            }

            *pcPos = 0;
        }
        else if (*pcPos == ':')
        {
            *pcPos = 0;
        }
    
        pcPos--;
    }

    pstNode = grub_zalloc(sizeof(netdrv_node));
    if (pstNode)
    {
        grub_snprintf(pstNode->szKoPath, sizeof(pstNode->szKoPath), "%s%s/%s", szMod, szKv, pcLine);

        

        
        file = grub_file_open(pstNode->szKoPath, VENTOY_FILE_TYPE);
        if (file)
        {
            pstNode->KoSize = (int)file->size;
            grub_file_close(file);

            if (g_pstNetDrvList)
            {
                g_pstNetDrvTail->pstNext = pstNode;
                g_pstNetDrvTail = pstNode;
            }
            else
            {
                g_pstNetDrvList = g_pstNetDrvTail = pstNode;
            }

            g_iNetDrvCount++;
            g_iNetDrvTotLen += pstNode->KoSize;            
        }
        else
        {
            grub_free(pstNode);
        }
    }

    free(pcLine);
    return 1;
}

static void vtoy_dump_netdrv(void)
{
    netdrv_node *pstNode = NULL;

    for (pstNode = g_pstNetDrvList; pstNode; pstNode = pstNode->pstNext)
    {
        printf("<%s> %d\n", pstNode->szKoPath, pstNode->KoSize);
    }
}

static void vtoy_free_netdrv(void)
{
    netdrv_node *pstNode = NULL;
    netdrv_node *pstNext = NULL;

    for (pstNode = g_pstNetDrvList; pstNode; pstNode = pstNext)
    {
        pstNext = pstNode->pstNext;
        grub_free(pstNode);
    }

    g_iNetDrvTotLen = 0;
    g_pstNetDrvList = g_pstNetDrvTail = NULL;
}

static char * vtoy_fill_netdrv_data(int *len, int base, int interval)
{
    int i = 0;
    int buflen;
    char *buf = NULL;
    grub_file_t file = NULL;
    netdrv_node *pstNode = NULL;
    netdrv_data_node *pstDrv = NULL;

    if (g_iNetDrvCount == 0)
    {
        return NULL;
    }

    buflen = g_iNetDrvTotLen + g_iNetDrvCount * (int)(sizeof(netdrv_data_node) + sizeof(uint32_t));
    buf = grub_malloc(buflen);
    if (buf)
    {
        *(uint32_t *)buf = 0x11223344;
        *(uint32_t *)(buf +4) = g_iNetDrvCount;
        pstDrv = (netdrv_data_node *)(buf + 8);
        for (pstNode = g_pstNetDrvList; pstNode; pstNode = pstNode->pstNext, i++)
        {
            vtoy_notify_progress(base + i * (interval / g_iNetDrvCount));
        
            pstDrv->uiFileLen = pstNode->KoSize;            
            grub_snprintf(pstDrv->szKoPath, sizeof(pstDrv->szKoPath), "%s", pstNode->szKoPath);

            file = grub_file_open(pstNode->szKoPath, VENTOY_FILE_TYPE);
            grub_file_read(file, pstDrv + 1, pstNode->KoSize);
            grub_file_close(file);

            pstDrv = (netdrv_data_node *)((char *)pstDrv + (sizeof(netdrv_data_node) +  pstNode->KoSize));
        }
    }

    *len = buflen;
    return buf;
}

static int vtoy_resolve_netdrv(char *szMod, char *szKv, uint8_t *pucPciID)
{
    int rc = 1;
    char szID[32];
    char szPath[256];
    char szSearch[256];
    char szSuffix[8] = { 0 };
    grub_file_t fdep = NULL;
    grub_file_t falias = NULL;
    char *pcModepBuf = NULL;
    char *pcModaliBuf = NULL;

    grub_snprintf(szPath, sizeof(szPath), "%s%s/modules.dep", szMod, szKv);
    fdep = grub_file_open(szPath, VENTOY_FILE_TYPE);
    
    grub_snprintf(szPath, sizeof(szPath), "%s%s/modules.alias", szMod, szKv);
    falias = grub_file_open(szPath, VENTOY_FILE_TYPE);
    
    if (NULL == fdep || NULL == falias)
    {
        vdebug("Failed to open file %s%s/modules.dep or modules.alias\n", szMod, szKv);
        grub_errno = 0;
        goto fail;
    }

    pcModepBuf = grub_malloc(fdep->size + 2);
    pcModaliBuf = grub_malloc(falias->size + 2);
    if (NULL == pcModepBuf || NULL == pcModaliBuf)
    {
        goto fail;
    }
    
    grub_file_read(fdep, pcModepBuf, fdep->size);
    pcModepBuf[fdep->size] = pcModepBuf[fdep->size + 1] = 0;
    vtoy_split_line(pcModepBuf);
    
    vtoy_get_ko_suffix(pcModepBuf, szSuffix);
    vdebug("KO suffix is <%s>\n", szSuffix);

    /* modules.alias */
    grub_file_read(falias, pcModaliBuf, falias->size);
    pcModaliBuf[falias->size] = pcModaliBuf[falias->size + 1] = 0;
    vtoy_split_line(pcModaliBuf);

    snprintf(szID, sizeof(szID), "%02x%02x%02x%02x", pucPciID[0], pucPciID[1], pucPciID[2], pucPciID[3]);

    /* Add netcard driver */
    if (strcmp(szID, "1af41000") == 0) /* virtio network */
    {
        grub_snprintf(szSearch, sizeof(szSearch), "/virtio_pci.ko%s:", szSuffix);
        vtoy_push_netdrv(pcModepBuf, szMod, szKv, szSearch);

        grub_snprintf(szSearch, sizeof(szSearch), "/virtio_net.ko%s:", szSuffix);
        vtoy_push_netdrv(pcModepBuf, szMod, szKv, szSearch);        
    }
    else if (vtoy_match_netdrv(pcModaliBuf, pucPciID, szPath, sizeof(szPath)) == 0)
    {
        grub_snprintf(szSearch, sizeof(szSearch), "/%s.ko%s:", szPath, szSuffix); 
        vtoy_push_netdrv(pcModepBuf, szMod, szKv, szSearch);
    }

    /* add nbd.ko */
    grub_snprintf(szSearch, sizeof(szSearch), "/nbd.ko%s:", szSuffix);
    vtoy_push_netdrv(pcModepBuf, szMod, szKv, szSearch);

    rc = 0;

fail:

    grub_check_free(pcModepBuf);
    grub_check_free(pcModaliBuf);
    grub_file_check_close(fdep);
    grub_file_check_close(falias);

    return rc;
}

static int vtoy_secondary_loop(char **aszArgv)
{
    int i;
    const char *aszSfs[] = 
    {
        "(a)/LiveOS/rootfs.img",
        "(a)/LiveOS/ext3fs.img",
        "(a)/root-image.fs",
        
        NULL
    };

    for (i = 0; aszSfs[i]; i++)
    {
        if (grub_file_exist(aszSfs[i]))
        {
            break;
        }
    }

    if (aszSfs[i])
    {
        aszArgv[0] = "loopback";
        aszArgv[1] = "b";
        aszArgv[2] = (char *)aszSfs[i];
        aszArgv[3] = NULL;
        grub_loopback_exec(4, aszArgv);

        return 1;
    }
    
    return 0;
}

static int vtoy_netdrv_sfs(int argc, char **argv)
{
    int i;
    int status = 0;
    int buflen = 0;
    grub_device_t dev = NULL;
    char *aszArgv[8] = { NULL };    
    char szModule[256] = { 0 };
    uint8_t aucPciID[8] = { 0 };
    SFS_KV_S *pstKvs = NULL;
    char *buf = NULL;
    grub_fs_t fs = NULL;

    vtoy_notify_progress(10);

    vdebug("netdrv sfs proc for <%s>\n", argv[1]);

    if ((!argv[1]) || (!grub_file_exist(argv[1])))
    {
        printf("File <%s> not found\n", argv[1]);
        return 1;
    }

    aszArgv[0] = "loopback";
    aszArgv[1] = "a";
    aszArgv[2] = argv[1];
    aszArgv[3] = NULL;
    grub_loopback_exec(4, aszArgv);

    if (vtoy_secondary_loop(aszArgv))
    {
        dev = grub_device_open("b");
        fs = grub_fs_probe(dev);
        if (NULL == dev || NULL == fs)
        {
            printf("Can not open device <%c> or unsupported fs. %p %p\n", szModule[1], dev, fs);
            return 1;
        }
        
        grub_snprintf(szModule, sizeof(szModule), "(b)/lib/modules/");
    }
    else
    {
        dev = grub_device_open("a");
        fs = &grub_squash_fs;
        if (NULL == dev)
        {
            printf("Can not open device <%c>\n", szModule[1]);
            return 1;
        }

        g_iSfsDirCount = 0;
        fs->fs_dir(dev, "/", vtoy_dir_sfs_enum, &status);

        vdebug("status=0x%x SfsDirCount=%d DirName=%s\n", status, g_iSfsDirCount, g_szSfsLastDirName);

        grub_snprintf(szModule, sizeof(szModule), "(a)/lib/modules/");

        if (status & DIR_STATUS_LIB_EXIST)
        {
            grub_snprintf(szModule, sizeof(szModule), "(a)/lib/modules/");            
        }
        else if (status & DIR_STATUS_MODULES_EXIST)
        {
            grub_snprintf(szModule, sizeof(szModule), "(a)/modules/");            
        }
        else if (status & DIR_STATUS_USR_EXIST)
        {
            status = 0;
            fs->fs_dir(dev, "/usr/lib", vtoy_dir_sfs_enum, &status);
            if (status & DIR_STATUS_MODULES_EXIST)
            {
                grub_snprintf(szModule, sizeof(szModule), "(a)/usr/lib/modules/"); 
            }
        }
        else if (status & DIR_STATUS_NIXOS_LAYOUT)
        {
            grub_snprintf(szModule, sizeof(szModule), "(a)/%s/lib/modules/", g_szNixOsModule);
        }
        else if (g_iSfsDirCount == 1)
        {
            if (grub_isdigit(g_szSfsLastDirName[0]))
            {
                grub_snprintf(szModule, sizeof(szModule), "(a)/");
            }
        }
    }

    pstKvs = grub_zalloc(sizeof(SFS_KV_S));
    if (!pstKvs)
    {
        grub_device_close(dev);
        return 1;
    }

    vdebug("Module=<%s>\n", szModule);
    fs->fs_dir(dev, szModule + 3, vtoy_sfs_enum, pstKvs);
    grub_device_close(dev);

    toy_setting_hex_get("busid", aucPciID, sizeof(aucPciID));

    vdebug("BUSID=<%02x%02x%02x%02x> Kernel count:%d\n", 
        aucPciID[1], aucPciID[2], aucPciID[3], aucPciID[4], pstKvs->kvs);

    vtoy_notify_progress(20);
        
    for (i = 0; i < pstKvs->kvs; i++)
    {    
        vdebug("kv %d : <%s><%s>\n", i, szModule, pstKvs->aszKv[i]);
        vtoy_notify_progress(20 + (i + 1) * (30 / pstKvs->kvs));
        vtoy_resolve_netdrv(szModule, pstKvs->aszKv[i], aucPciID + 1);
    }

    vtoy_notify_progress(50);

    if (g_debug_flag && g_pstNetDrvList)
    {
        vdebug("========dump net drv list=========\n");
        vtoy_dump_netdrv();        
        vdebug("==================================\n");
    }

    vtoy_notify_progress(70);
    
    vdebug("Load net drv files... ");
    buf = vtoy_fill_netdrv_data(&buflen, 70, 25);
    if (buf)
    {
        g_pcSandiskOverBuf = buf;
        g_uiSandiskOverLen = buflen;
        for (i = 2; i < argc && g_aiSandiskOverCnt < 16; i++)
        {
            g_aui64SandiskOverLoc[g_aiSandiskOverCnt] = strtoul(argv[i], NULL, 10);
            g_aiSandiskOverCnt++;
        }
    }
    vdebug(" OK\n");

    vtoy_free_netdrv();
    grub_free(pstKvs);
    
    return 0;
}

static int vtoy_udeb_enum(const char *filename, const struct grub_dirhook_info *info, void *data)
{
    grub_file_t file = NULL;
    netdrv_node *pstNode = NULL;

    if (info->dir == 1)
    {
        return 0;
    }

    if (strncmp(filename, "nbd-modules-", 12) == 0 ||
        strncmp(filename, "nic-modules-", 12) == 0 ||
        strncmp(filename, "nic-shared-modules-", 19) == 0 ||
        strncmp(filename, "nic-usb-modules-", 16) == 0)
    {
        pstNode = grub_zalloc(sizeof(netdrv_node));
        if (pstNode)
        {
            grub_snprintf(pstNode->szKoPath, sizeof(pstNode->szKoPath), "%s/%s", (char *)data, filename);
            file = grub_file_open(pstNode->szKoPath, VENTOY_FILE_TYPE);
            pstNode->KoSize = (int)file->size;
            grub_file_close(file);

            if (g_pstNetDrvList)
            {
                g_pstNetDrvTail->pstNext = pstNode;
                g_pstNetDrvTail = pstNode;
            }
            else
            {
                g_pstNetDrvList = g_pstNetDrvTail = pstNode;
            }

            g_iNetDrvCount++;
            g_iNetDrvTotLen += pstNode->KoSize;
        }
    }

    return 0;
}

static int vtoy_netdrv_udeb(int argc, char **argv)
{
    int i;
    int rc = 1;
    int buflen = 0;
    grub_device_t dev = NULL;
    grub_fs_t fs = NULL;
    char *pcPos = NULL;
    char *buf = NULL;

    vdebug("netdrv udeb proc for <%s>\n", argv[1]);

    vtoy_notify_progress(20);

    pcPos = grub_file_get_device_name(argv[1]);
    dev = grub_device_open(pcPos);
    grub_free(pcPos);

    check_goto_end(dev, "Failed to open device <%s>\n", argv[1]);

    
    fs = grub_fs_probe(dev);
    check_goto_end(fs, "Failed to probe fs for device <%s>\n", argv[1]);

    pcPos = strchr(argv[1], '/');
    if (pcPos)
    {
        fs->fs_dir(dev, pcPos, vtoy_udeb_enum, argv[1]);        
    }

    vtoy_notify_progress(50);

    if (g_debug_flag && g_pstNetDrvList)
    {
        vdebug("========dump net drv list=========\n");
        vtoy_dump_netdrv();        
        vdebug("==================================\n");
    }

    vtoy_notify_progress(70);
    
    vdebug("Load net drv files... ");
    buf = vtoy_fill_netdrv_data(&buflen, 70, 25);
    if (buf)
    {
        g_pcSandiskOverBuf = buf;
        g_uiSandiskOverLen = buflen;
        for (i = 2; i < argc && g_aiSandiskOverCnt < 16; i++)
        {
            g_aui64SandiskOverLoc[g_aiSandiskOverCnt] = strtoul(argv[i], NULL, 10);
            g_aiSandiskOverCnt++;
        }
    }
    vdebug(" OK\n");

    vtoy_free_netdrv();
    
    rc = 0;
    
end:

    if (dev)
    {
        grub_device_close(dev);        
    }

    return rc;
}

int vtoy_netdrv_exec(int argc, char **argv)
{
    int rc = 0;
    char szType[16] = { 0 };

    toy_setting_string_get("bustype", szType, sizeof(szType));
    if (strcasecmp(szType, "PCI"))
    {
        printf("BUS type <%s> is not supported\n", szType);
        rc = 1;
    }
    else if (strcmp(argv[1], "sfs") == 0)
    {
        rc = vtoy_netdrv_sfs(argc - 1, argv + 1);
    }
    else if (strcmp(argv[1], "udeb") == 0)
    {
        rc = vtoy_netdrv_udeb(argc - 1, argv + 1);
    }

    vtoy_notify_progress(100);    

    ventoy_debug_pause();
    return rc;
}

static int vtoy_check_pwd(char *pwd)
{
    int i;
    char *line = NULL;
    char szMD5[64];
    
    for (i = 0; i < 3; i++)
    {
        g_password_flag = 1;
        line = readline("Enter password: ");
        g_password_flag = 0;
        
        if (!line)
        {
            continue;
        }

        vtoy_calc_md5(line, (int)strlen(line), szMD5);
        free(line);

        if (strcasecmp(pwd, szMD5) == 0)
        {
            return 0;
        }
        else
        {
            printf("Incorrect password!\n\n");
        }
    }

    return 1;
}

int vtoy_check_sys_pwd_exec(int argc, char **argv)
{
    char szVer[32];
    char szArch[32];

    if (argc != 2)
    {
        return 1;
    }

    toy_setting_string_get("TOY_ARCH", szArch, sizeof(szArch));
    toy_setting_string_get("TOY_VER", szVer, sizeof(szVer));

    system("console -x 1024 -y 768");
    printf("====== iVentoy %s %s  www.iventoy.com ======\n\n", szVer, szArch);
    if (vtoy_check_pwd(argv[1]) == 0)
    {
        return 0;
    }
    printf("\n\n!!! Password check failed, will exit after 5 seconds. !!!\n");
    sleep(5);

    return 1;
}

int vtoy_check_file_pwd_exec(int argc, char **argv)
{
    int ret;
    
    if (argc != 2)
    {
        return 1;
    }

    system("console -x 1024 -y 768");
    ret = vtoy_check_pwd(argv[1]);
    system("vtoy_last_console");

    return ret;
}

int vtoy_efi_replace_exec(int argc, char **argv)
{
    int i;
    
    if (argc != 4)
    {
        return 1;
    }

    memset(g_szEFIReplace, 0, sizeof(g_szEFIReplace));
    for (i = 0; i < 128 && argv[1][i]; i++)
    {
        g_szEFIReplace[i] = argv[1][i];
        if (g_szEFIReplace[i] == '/')
        {
            g_szEFIReplace[i] = '\\';
        }
    }

    g_uiReplaceSector = (uint32_t)strtoul(argv[2], NULL, 10);
    g_uiReplaceSize = (uint32_t)strtoul(argv[3], NULL, 10);

    return 0;
}


int vtoy_check_timeout(int argc, char **argv)
{
    (void)argc;
    (void)argv;

    if (g_timeout_flag)
    {
        g_timeout_flag = 0;
        return 0;
    }

    return 1;
}


int grub_ttt_exec(int argc, char **argv)
{
    char *line = NULL;

    (void)argc;
    (void)argv;

    g_password_flag = 1;
    line = readline("Enter password: ");
    free(line);
    
    printf("Line=<%s>\n", line);
    g_password_flag = 0;

    return 0;
}

int grub_md5sum_exec(int argc, char **argv)
{
    int i;
    grub_uint64_t totlen = 0;
    grub_uint64_t dstlen = 0;
    grub_size_t readlen = 0;
    grub_file_t file = NULL;
    grub_uint8_t *buf = NULL;
    grub_uint8_t ctx[MD5_CTX_SIZE];
    grub_uint8_t result[MD5_DIGEST_SIZE];

    grub_errno = 0;

    argshift();
    
    if (argv[0] && strcmp(argv[0], "-n") == 0)
    {
        dstlen = strtoul(argv[1], NULL, 10);        
        argshift();
        argshift();
    }

    buf = grub_malloc(FILE_BUF_SIZE);
    if (!buf)
    {
        printf("Failed to alloc file buffer\n");
        return 1;
    }

    file = grub_file_open(argv[0], VENTOY_FILE_TYPE);
    if (!file)
    {
        printf("Failed to open file <%s>\n", argv[0]);
        grub_free(buf);
        return 1;
    }

    if (dstlen == 0)
    {
        dstlen = file->size;
    }

    digest_init(&md5_algorithm, ctx);

    while (totlen < dstlen)
    {
        if (dstlen - totlen >= FILE_BUF_SIZE)
        {
            readlen = grub_file_read(file, buf, FILE_BUF_SIZE);            
        }
        else
        {
            readlen = grub_file_read(file, buf, dstlen - totlen);            
        }

        totlen += readlen;
        digest_update(&md5_algorithm, ctx, buf, readlen);
    }

    digest_final(&md5_algorithm, ctx, result);

    printf("MD5 : ");
    for (i = 0; i < (int)MD5_DIGEST_SIZE; i++)
    {
        printf("%02x",  result[i]);
    }
    printf("\n");

    printf("size: %llu\n", (_ull)file->size);
    printf("file: <%s>\n", argv[0]);

    grub_free(buf);
    grub_file_close(file);

    return 0;
}

int grub_crc32_exec(int argc, char **argv)
{
    grub_uint32_t crc = 0;
    grub_uint64_t totlen = 0;
    grub_uint64_t dstlen = 0;
    grub_size_t readlen = 0;
    grub_file_t file = NULL;
    grub_uint8_t *buf = NULL;

    grub_errno = 0;

    argshift();
    
    if (argv[0] && strcmp(argv[0], "-n") == 0)
    {
        dstlen = strtoul(argv[1], NULL, 10);        
        argshift();
        argshift();
    }

    buf = grub_malloc(FILE_BUF_SIZE);
    if (!buf)
    {
        printf("Failed to alloc file buffer\n");
        return 1;
    }

    file = grub_file_open(argv[0], VENTOY_FILE_TYPE);
    if (!file)
    {
        printf("Failed to open file <%s>\n", argv[0]);
        grub_free(buf);
        return 1;
    }

    if (dstlen == 0)
    {
        dstlen = file->size;
    }

    while (totlen < dstlen)
    {
        if (dstlen - totlen >= FILE_BUF_SIZE)
        {
            readlen = grub_file_read(file, buf, FILE_BUF_SIZE);            
        }
        else
        {
            readlen = grub_file_read(file, buf, dstlen - totlen);            
        }

        totlen += readlen;
        crc = grub_getcrc32c(crc, buf, readlen);
    }

    printf("crc32: %08X\n", crc);
    printf("size: %llu\n", (_ull)file->size);
    printf("file: <%s>\n", argv[0]);

    grub_free(buf);
    grub_file_close(file);

    return 0;
}


struct command grub_commands[] __command = {
    {
		.name = "vtdebug",
		.exec = grub_debug_exec,
	},
    {
		.name = "ls",
		.exec = grub_ls_exec,
	},
    {
		.name = "loopback",
		.exec = grub_loopback_exec,
	},
    {
		.name = "cat",
		.exec = grub_cat_exec,
	},
    {
		.name = "crc64",
		.exec = grub_hashsum_exec,
	},
    {
		.name = "crc32",
		.exec = grub_crc32_exec,
	},
    {
		.name = "md5sum",
		.exec = grub_md5sum_exec,
	},
    {
		.name = "ttt",
		.exec = grub_ttt_exec,
	},
    {
		.name = "hexdump",
		.exec = grub_hexdump_exec,
	},
    {
		.name = "vtoy_console",
		.exec = vtoy_console,
	},
    {
		.name = "vtoy_last_console",
		.exec = vtoy_last_console,
	},
    {
		.name = "vtoy_sys_pwd",
		.exec = vtoy_check_sys_pwd_exec,
	},
    {
		.name = "vtoy_file_pwd",
		.exec = vtoy_check_file_pwd_exec,
	},
    {
		.name = "vtoy_chk_timeout",
		.exec = vtoy_check_timeout,
	},
    {
		.name = "vtoy_efi_replace",
		.exec = vtoy_efi_replace_exec,
	},
    {
		.name = "netdrv",
		.exec = vtoy_netdrv_exec,
	}
};


