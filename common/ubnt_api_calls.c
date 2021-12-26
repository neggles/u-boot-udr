// vim: ts=4:sw=4:expandtab
/*          Copyright Ubiquiti Networks Inc. 2014
**          All Rights Reserved.
*/

#include <common.h>
#include <command.h>
#include <image.h>
#include <asm/string.h>
#include <ubnt_export.h>
#include <version.h>
#include <net.h>
#if defined(CONFIG_FIT) || defined(CONFIG_OF_LIBFDT)
    #include <fdt.h>
    #include <libfdt.h>
#endif

#define SPI_FLASH_PAGE_SIZE             0x1000

/* header size comes from create_header() in mkheader.ph */
#define UBOOT_IMAGE_HEADER_SIZE     40

#define UBNT_APP_MAGIC 0x87564523
/* these values come from ubnt.lds */
#define UBNT_APP_IMAGE_OFFSET           0x4007ff28
#define UBNT_APP_MAGIC_OFFSET           UBNT_APP_IMAGE_OFFSET
#define UBNT_APP_SHARE_DATA_OFFSET      0x4007ff48
#define UBNT_APP_START_OFFSET           0x4007ffc8

#define UBNT_FLASH_SECTOR_SIZE          (250 * SPI_FLASH_PAGE_SIZE)
#define UBNT_FWUPDATE_SECTOR_SIZE       (16 * SPI_FLASH_PAGE_SIZE)

#define MAX_BOOTARG_STRING_LENGTH 512
#define ENV_BOOTARG "bootargs"
#define UBOOT_VER_ARG_STR "ubootver="

/* ipq related */
/*
 * SOC version type with major number in the upper 16 bits and minor
 * number in the lower 16 bits.  For example:
 *   1.0 -> 0x00010000
 *   2.3 -> 0x00020003
 */
#define SOCINFO_VERSION_MAJOR(ver) ((ver & 0xffff0000) >> 16)
#define SOCINFO_VERSION_MINOR(ver) (ver & 0x0000ffff)

#define KERNEL_VALIDATION_UNKNOW    0
#define KERNEL_VALIDATION_VALID     1
#define KERNEL_VALIDATION_INVALID   2

#define IMAGE_FORMAT_LEGACY     0x01    /* legacy image_header based format */

DECLARE_GLOBAL_DATA_PTR;
static int ubnt_debug = 0;
static unsigned int soc_version = 1;
#define DTB_CFG_LEN     64
#if defined(CONFIG_FIT)
static char dtb_config_name[DTB_CFG_LEN];
#endif

static ubnt_app_share_data_t * p_share_data = (ubnt_app_share_data_t *)UBNT_APP_SHARE_DATA_OFFSET;

static inline unsigned int spi_flash_addr(unsigned int off)
{
    return off + CFG_FLASH_BASE;
}

static int append_bootargs(char * fmt, ...)
{
    int w_idx = 0;
    char *env_var_str;
    va_list args;
    char bootargs[MAX_BOOTARG_STRING_LENGTH];

    env_var_str = getenv(ENV_BOOTARG);

    if(env_var_str) {
        w_idx += strlen(env_var_str);
        if (w_idx >= sizeof(bootargs) - 1) {
            printf("**WARNING**, bootargs do not fit in buffer.\n");
            return CMD_RET_FAILURE;
        }
        // There appears to be a bug in vsnprintf, such that calling it after snprintf
        // causes no string data to be appended. Use strcpy instead.
        strcpy(bootargs, env_var_str);
        bootargs[w_idx++] = ' ';
        bootargs[w_idx] = '\0';
    }

    va_start(args, fmt);
    w_idx += vsnprintf(bootargs + w_idx, (size_t)(sizeof(bootargs) - w_idx), fmt, args);
    va_end(args);

    if (w_idx >= sizeof(bootargs)) {
        printf("**WARNING**, cannot append bootargs, buffer too small.\n");
        return CMD_RET_FAILURE;
    }

    setenv(ENV_BOOTARG, bootargs);

    return CMD_RET_SUCCESS;
}

void load_sernum_ethaddr(void)
{
    char buf[64];
    uchar *eeprom = spi_flash_addr(UBNT_PART_EEPROM_OFF);
    /* Use default address if the eeprom address is invalid */
    if (eeprom[0] & 0x01 || memcmp(eeprom, "\0\0\0\0\0\0", 6) == 0)
    {
        return;
    }
    snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
            eeprom[0], eeprom[1], eeprom[2],
            eeprom[3], eeprom[4], eeprom[5]);
    setenv("ethaddr", buf);
}

typedef struct {
    ubnt_ramload_t *ramload;
    int is_valid;
    unsigned int addr;
} ramload_state_t;

static ramload_state_t ramload_state;

unsigned long ubnt_ramload_init(unsigned long addr, int update_state)
{
    ubnt_ramload_t *ramload;
    unsigned int xor;
    int is_valid;
    addr -= sizeof(ubnt_ramload_placeholder_t);
    ramload = (ubnt_ramload_t *)addr;
    xor = ramload->magic ^ ramload->size;
    is_valid = (ramload->magic == UBNT_RAMLOAD_MAGIC
                && ramload->xor_m_s == xor);
    if (update_state) {
        ramload_state.is_valid = is_valid;
        ramload_state.ramload = ramload;
    }
    if (is_valid) {
        printf("UBNT Ramload image of size 0x%x found\n", ramload->size);
        addr -= ramload->size;
        if (update_state) {
            ramload_state.addr = addr;
        }
    }
    return addr;
}

/* Loads the application into memory */
int ubnt_app_load(void)
{
    char addr[10];
    char runcmd[256];
    unsigned int ubntapp_size;
    unsigned int current_write_addr = UBNT_APP_IMAGE_OFFSET;
    unsigned int current_read_addr;
    unsigned int magic_value;
    int ret;

    current_read_addr = 0x60000 + monitor_flash_len + 512;
    ubntapp_size = 0x60000 - monitor_flash_len;

    snprintf(runcmd, sizeof(runcmd),
             "snor read 0x%x 0x%x 0x%x",
             current_write_addr, current_read_addr, ubntapp_size);
    ret = run_command(runcmd, 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: flash read error (aligned bytes), command: %s, ret: %d, read addr=%x, write addr=%x \n",
                runcmd,
                ret,
                current_read_addr,
                current_write_addr);
        return -1;
    }

    magic_value = *((unsigned int *)UBNT_APP_MAGIC_OFFSET);
    if ( UBNT_APP_MAGIC != htonl(magic_value) ) {
        printf(" *WARNING*: UBNT APP Magic mismatch, src=%x, addr=%x, magic=%x \n",
               current_read_addr,
               current_write_addr,
               magic_value);
        return -1;
    }

    sprintf(addr, "%x", UBNT_APP_START_OFFSET);
    setenv("ubntaddr", addr);

    return 0;
}

static void erase_spi_flash(int offset, int size) {
    char runcmd[256];
    int ret;

    snprintf(runcmd, sizeof(runcmd), "snor erase 0x%x 0x%x",
             offset, size);
    if (ubnt_debug) {
        printf("DEBUG, erase spi flash command: %s\n", runcmd);
    }
    ret = run_command(runcmd, 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: flash erase error, command: %s, ret: %d \n", runcmd, ret);
    }
}

#if defined(CONFIG_FIT)
/* Ported from boot_get_kernel() of cmd_bootm.c. */
static int ubnt_validate_fit_kernel(const void *fit)
{
    int image_valid = KERNEL_VALIDATION_INVALID;
    const char *fit_uname_config = NULL;
    const char *fit_uname_kernel = NULL;
    int cfg_noffset;
    int os_noffset;

    if (!fit_check_format(fit)) {
        printf("UBNT, Bad FIT kernel image format.\n");
        return image_valid;
    }

    cfg_noffset = fit_conf_get_node(fit, fit_uname_config);
    if (cfg_noffset < 0) {
        printf("UBNT, No config found in FIT kernel image.\n");
        return image_valid;
    }

    // jeffrey
    //os_noffset = fit_conf_get_kernel_node(fit, cfg_noffset);
    fit_uname_kernel = fit_get_name(fit, os_noffset, NULL);

    if (os_noffset < 0) {
        printf("UBNT, No config found in FIT kernel image.\n");
        return image_valid;
    }

    /* check kernel */
#if 0 // jeffrey
    if (!fit_image_check_hashes(fit, os_noffset)) {
        printf("UBNT, Bad Data Hash for FIT kernel image.\n");
        return image_valid;
    }

    if (!fit_image_check_target_arch(fit, os_noffset)) {
        printf("UBNT, Unsupported Architecture for FIT kernel image.\n");
        return image_valid;;
    }

    if (!fit_image_check_type(fit, os_noffset, IH_TYPE_KERNEL) &&
        !fit_image_check_type(fit, os_noffset, IH_TYPE_KERNEL_NOLOAD)) {
        printf("UBNT, Not a kernel image\n");
        return image_valid;
    }
#endif

    image_valid = KERNEL_VALIDATION_VALID;
    return image_valid;
}
#endif /* CONFIG_FIT */

/* This function only validate FIT kernel image for now. */
static int ubnt_validate_kernel_image(const void * img_addr) {
    int image_valid = KERNEL_VALIDATION_UNKNOW;
    int image_type = genimg_get_format(img_addr);

    switch (image_type) {
#if defined(CONFIG_FIT)
    case IMAGE_FORMAT_FIT:
        image_valid = ubnt_validate_fit_kernel(img_addr);
        break;
#endif
    default:
        /* legacy image is validated by ubnt_app */
        break;
    }

    return image_valid;
}

static int get_image_size(unsigned int img_addr, int *size) {
    image_header_t *hdr;
    char *fdt;
    int image_type;
    int ret = -1;

    image_type = genimg_get_format((void *)img_addr);
    //printf("immage address: 0x%08x, type: %d\n", img_addr, image_type);
    switch (image_type) {
    case IMAGE_FORMAT_LEGACY:
        hdr = (image_header_t *)img_addr;
        if (uimage_to_cpu(hdr->ih_magic) == IH_MAGIC)
        {
            *size = uimage_to_cpu(hdr->ih_size) + sizeof(image_header_t);
            ret = 0;
        }
        break;
#if defined(CONFIG_FIT)
    case IMAGE_FORMAT_FIT:
        fdt = (char *)img_addr;
        *size = fdt_totalsize(fdt);
        ret = 0;
        break;
#endif
    default:
        break;
    }

    return ret;
}

static int ubnt_read_kernel_image(void) {
    unsigned int load_addr = CONFIG_SYS_LOAD_ADDR;
    unsigned int current_offset;
    int current_size, size_left, img_size = 0;
    unsigned int offset;
    unsigned int size;
    char runcmd[256];
    int ret;
    int image_valid = KERNEL_VALIDATION_INVALID;

    if (p_share_data->kernel_index != 0) {
        offset = UBNT_PART_KERNEL1_OFF;
        size = UBNT_PART_KERNEL1_SIZE;
    } else {
        offset = UBNT_PART_KERNEL0_OFF;
        size = UBNT_PART_KERNEL0_SIZE;
    }

    current_offset = offset;
    size_left = (int)size;
    current_size = (size_left < SPI_FLASH_PAGE_SIZE) ? size_left : SPI_FLASH_PAGE_SIZE;

    /* read first page to find f/w size */
    snprintf(runcmd, sizeof(runcmd), "snor read 0x%x 0x%x 0x%x", load_addr, current_offset, current_size);
    if (ubnt_debug) {
        printf("read first page %x %x %x\n", load_addr, current_offset, current_size);
    }
    ret = run_command(runcmd, 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: flash read error (Kernel), command: %s, ret: %d \n", runcmd, ret);
        return -1;
    }
    ret = get_image_size(load_addr, &img_size);
    if (ret == 0) {
        size_left = img_size < size ? img_size : size;
        /* align to page size */
        size_left = (size_left + SPI_FLASH_PAGE_SIZE - 1) & (~(SPI_FLASH_PAGE_SIZE - 1));
    } else {
        return -1;
    }
    printf("reading kernel %d from: 0x%x, size: 0x%08x\n", p_share_data->kernel_index, offset, size_left);

    size_left -= current_size;
    load_addr += current_size;
    current_offset += current_size;

    while ( size_left > 0 ) {
        if ( size_left > UBNT_FLASH_SECTOR_SIZE) {
            current_size = UBNT_FLASH_SECTOR_SIZE;
        } else {
            current_size = size_left;
        }
        if (ubnt_debug) {
            printf("snor read 0x%x 0x%x 0x%x\n", load_addr, current_offset, current_size);
        }
        snprintf(runcmd, sizeof(runcmd), "snor read 0x%x 0x%x 0x%x", load_addr, current_offset, current_size);
        ret = run_command(runcmd, 0);
        if (ret != CMD_RET_SUCCESS) {
            printf(" *WARNING*: flash read error (Kernel), command: %s, ret: %d \n", runcmd, ret);
            return image_valid;
        }

        ubnt_app_continue(UBNT_EVENT_READING_FLASH);
        size_left -= current_size;
        load_addr += current_size;
        current_offset += current_size;
    }

    p_share_data->kernel_addr = CONFIG_SYS_LOAD_ADDR;
    image_valid = ubnt_validate_kernel_image((void *)p_share_data->kernel_addr);

    if (ubnt_debug) {
        printf("UBNT, read kernel image done, kernel valid: %d.\n", image_valid);
    }

    return image_valid;
}

static int ubnt_update_partition(
    const char * partition_name,
    unsigned int partition_offset,
    unsigned int partition_size,
    unsigned int image_address,
    unsigned int image_size
    )
{
    char runcmd[256];
    int ret;

    printf("Updating %s partition (and skip identical blocks) ...... ", partition_name);
    if (image_size > partition_size) {
        printf("Cannot update partition %s with image size %x. "
               "Partition is only %x.\n",
               partition_name, image_size, partition_size);
        return -1;
    }
    snprintf(runcmd, sizeof(runcmd), "snor erase 0x%x 0x%x", partition_offset, partition_size);
    if(ubnt_debug) {
        printf("DEBUG, update uboot command: %s\n", runcmd);
    }
    ret = run_command(runcmd, 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: flash update error (%s), command: %s, ret: %d \n", partition_name, runcmd, ret);
        return -1;
    }

    snprintf(runcmd, sizeof(runcmd), "snor write 0x%x 0x%x 0x%x", image_address, partition_offset, image_size);
    if(ubnt_debug) {
        printf("DEBUG, update uboot command: %s\n", runcmd);
    }
    ret = run_command(runcmd, 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: flash update error (%s), command: %s, ret: %d \n", partition_name, runcmd, ret);
        return -1;
    }

    printf("done.\n");

    return 0;
}

static int ubnt_update_uboot_image(void)
{
    return ubnt_update_partition(
        UBNT_PART_UBOOT_NAME,
        UBNT_PART_UBOOT_OFF,
        UBNT_PART_UBOOT_SIZE,
        p_share_data->update.uboot.addr,
        p_share_data->update.uboot.size
        );
}

static int ubnt_update_bootselect(void)
{
    return ubnt_update_partition(
        UBNT_PART_BOOTSELECT_NAME,
        UBNT_PART_BOOTSELECT_OFF,
        UBNT_PART_BOOTSELECT_SIZE,
        p_share_data->update.bootselect.addr,
        p_share_data->update.bootselect.size
        );
}

static int ubnt_update_kernel_image(int kernel_index)
{
    unsigned int load_addr;
    unsigned int current_offset, current_size, size_left;
    char runcmd[256];
    const char * partition_name = NULL;
    int ret;

    if (kernel_index != 0) {
        partition_name = UBNT_PART_KERNEL1_NAME;
        current_offset = UBNT_PART_KERNEL1_OFF;
        load_addr = p_share_data->update.kernel1.addr;
        size_left = p_share_data->update.kernel1.size;
    } else {
        partition_name = UBNT_PART_KERNEL0_NAME;
        current_offset = UBNT_PART_KERNEL0_OFF;
        load_addr = p_share_data->update.kernel0.addr;
        size_left = p_share_data->update.kernel0.size;
    }

    printf("Updating %s partition (and skip identical blocks) ...... ", partition_name);
    while( size_left > 0 ) {
        if( size_left > UBNT_FWUPDATE_SECTOR_SIZE) {
            current_size = UBNT_FWUPDATE_SECTOR_SIZE;
        } else {
            current_size = size_left;
        }

        snprintf(runcmd, sizeof(runcmd), "snor erase 0x%x 0x%x", current_offset, UBNT_FWUPDATE_SECTOR_SIZE);
        ret = run_command(runcmd, 0);
        if (ret != CMD_RET_SUCCESS) {
            printf(" *WARNING*: flash update error (%s), command: %s, ret: %d \n", partition_name, runcmd, ret);
            return -1;
        }

        snprintf(runcmd, sizeof(runcmd), "snor write 0x%x 0x%x 0x%x", load_addr, current_offset, current_size);
        ret = run_command(runcmd, 0);
        if (ret != CMD_RET_SUCCESS) {
            printf(" *WARNING*: flash update error (%s), command: %s, ret: %d \n", partition_name, runcmd, ret);
            return -1;
        }

        ubnt_app_continue(UBNT_EVENT_WRITING_FLASH);
        size_left -= current_size;
        load_addr += current_size;
        current_offset += current_size;
    }
    printf("done.\n");

    if(ubnt_debug) {
        printf("UBNT, update kernel done.\n");
    }

    return 0;
}

int ubnt_app_start(int argc, char *const argv[]) {
    char *ubnt_flag;
    char tmp_str[64];
    int ret, i;
    int ubnt_app_event;
    int do_urescue = 0;

    ret = run_command("nor init", 0);
    if (ret != CMD_RET_SUCCESS) {
        printf(" *WARNING*: nor init failed, ret: %d.\n", ret);
        return -1;
    }

    ret = ubnt_app_load();
    if (ret != 0) {
        return -1;
    }

    snprintf(tmp_str, sizeof(tmp_str), "go ${ubntaddr} uappinit");
    for (i = 0; i < argc; i++) {
        snprintf(tmp_str, sizeof(tmp_str), "%s %s", tmp_str, argv[i]);
    }

    if(ubnt_debug) {
        printf("UBNT, init command: %s\n", tmp_str);
    }

    ret = run_command(tmp_str, 0);
    if (ret != 0) {
        /* do not return here to give urescue a chance to run */
        printf(" *WARNING*: uappinit failed, ret: %d.\n", ret);
        return -1;
    }

    if (ramload_state.is_valid) {
        snprintf(tmp_str, sizeof(tmp_str), "go ${ubntaddr} uramload 0x%x 0x%x",
                ramload_state.addr, ramload_state.ramload->size);
        ret = run_command(tmp_str, 0);
        if (ret == 0) {
            append_bootargs("ubntramloaded");
            printf("Booting from ramload\n");
            return 0;
        }
    }

    ubnt_app_event = UBNT_EVENT_UNKNOWN;

    while(1) {
        snprintf(tmp_str, sizeof(tmp_str),
                "go ${ubntaddr} uappcontinue %d", ubnt_app_event);
        run_command(tmp_str, 0);
        ubnt_app_event = UBNT_EVENT_UNKNOWN;

        ubnt_flag = getenv("ubnt_do_boot");
        if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
            break;
        }

        ubnt_flag = getenv("do_urescue");
        if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
            do_urescue = 1;
            break;
        } else {
            ret = ubnt_read_kernel_image();

            if (ret == KERNEL_VALIDATION_INVALID) {
                ubnt_app_event = UBNT_EVENT_KERNEL_INVALID;
            } else if (ret == KERNEL_VALIDATION_VALID) {
                ubnt_app_event = UBNT_EVENT_KERNEL_VALID;
            }
        }
    }

    ubnt_flag = getenv("ubnt_clearcfg");
    if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
        printf("Erasing %s partition ...... ", UBNT_PART_CFG_NAME);
        erase_spi_flash(UBNT_PART_CFG_OFF,
                        UBNT_PART_CFG_SIZE);
        printf("done.\n");
    }

    ubnt_flag = getenv("ubnt_clearenv");
    if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
        printf("Erasing %s partition ...... ", UBNT_PART_UBOOT_ENV_NAME);
        // jeffrey
        //clearenv();
        printf("done.\n");
    }

    if (do_urescue) {
        sprintf(tmp_str, "0x%x", CONFIG_SYS_LOAD_ADDR);
        setenv("loadaddr", tmp_str);
        while(1) {
            run_command("tftpsrv", 0);
            ubnt_app_continue(UBNT_EVENT_TFTP_DONE);
            ubnt_flag = getenv("ubnt_do_boot");
            if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
                break;
            }
        }
        /* has done urescue, need to do following
        * 1) update kernel 0
        * 2) erase vendor data partition
        * 3) erase cfg partition
        * 4) erase uboot environment variables
        */
        if(ubnt_debug) {
            printf("addresses, kernel0: 0x%08x, kernel1: 0x%08x, uboot: 0x%08x, "
                   "bootselect: 0x%08x\n",
                   p_share_data->update.kernel0.addr,
                   p_share_data->update.kernel1.addr,
                   p_share_data->update.uboot.addr,
                   p_share_data->update.bootselect.addr
            );
        }

        if (p_share_data->update.uboot.addr != UBNT_INVALID_ADDR) {
            ubnt_update_uboot_image();
        }
        if (p_share_data->update.bootselect.addr != UBNT_INVALID_ADDR) {
            ubnt_update_bootselect();
        }
        if (p_share_data->update.kernel0.addr != UBNT_INVALID_ADDR) {
            ubnt_update_kernel_image(0);
        }
        if (p_share_data->update.kernel1.addr != UBNT_INVALID_ADDR) {
            ubnt_update_kernel_image(1);
        }

        /* reset system */
        run_command("reset", 0);
        return 0;
    }

    return 0;
}

int ubnt_is_kernel_loaded(void) {
    char *ubnt_flag;
    int ret;
    ubnt_flag = getenv("ubnt_do_boot");
    if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
        ret = 1;
    } else {
        ret = 0;
    }

    return ret;
}

void ubnt_app_continue(int event) {
    char runcmd[128];
    char *ubnt_flag;

    ubnt_flag = getenv("ubntaddr");
    if (ubnt_flag) {
        snprintf(runcmd, sizeof(runcmd),
                "go ${ubntaddr} uappcontinue %d", event);
        run_command(runcmd, 0);
    } else {
        if(ubnt_debug) {
            printf("ubnt_app_continue, ubntaddr not set. \n");
        }
    }
}

/**
 * Load Kernel image and transfer control to kernel
 */
static int load_kernel(unsigned int addr)
{
    char runcmd[256];
#if defined(CONFIG_FIT)
    unsigned char bootconfig;
#endif

    if (ubnt_debug) {
        run_command("printenv bootargs", 0);
        printf("Booting from flash\n");
    }

#if defined(CONFIG_FIT)
    bootconfig = simple_strtol(getenv("bootconfig"), NULL, 0);

    if (bootconfig <= 0) {
        return CMD_RET_FAILURE;
    }

    snprintf(dtb_config_name,
                sizeof(dtb_config_name),"#config@%d", bootconfig);
    snprintf(runcmd, sizeof(runcmd),"bootm 0x%x%s\n",
        addr,
        dtb_config_name);
#else /* CONFIG_FIT */
    snprintf(runcmd, sizeof(runcmd),"bootm 0x%x\n", addr);
#endif /* CONFIG_FIT */

    if (ubnt_debug)
        printf(runcmd);

    if (run_command(runcmd, 0) != CMD_RET_SUCCESS) {
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

/**
 * set boot argument: uboot version
 */
static void set_bootargs_uboot_version(void) {
    const char uboot_version[] = U_BOOT_VERSION;
    char *ver_start, *ver_end_delimiter;
    int ver_str_len;

    /* uboot version example:
     * U-Boot 2012.07-00069-g7963d5b-dirty [UniFi,ubnt_feature_ubntapp.69]
     */
    ver_start = strchr(uboot_version, ',') + 1;
    ver_end_delimiter = strchr(uboot_version, ']');
    ver_str_len = ver_end_delimiter - ver_start;
    if (append_bootargs("%s%.*s", UBOOT_VER_ARG_STR, ver_str_len, ver_start)
            != CMD_RET_SUCCESS) {
        printf("**WARNING**, buffer too small for uboot version bootargs.\n");
    }

    if (ubnt_debug) {
        printf("uboot version: %s\nbootargs: %s\n", uboot_version, getenv(ENV_BOOTARG));
    }
}

#ifdef UBNT_PERSISTENT_LOG
static void set_bootargs_ramoops(int * pmem_size) {
    *pmem_size -= UBNT_LOG_BUFF_SIZE;


    if (append_bootargs("ramoops.mem_address=0x%x ramoops.mem_size=%d ramoops.ecc=1",
            *pmem_size,
            UBNT_LOG_BUFF_SIZE) != CMD_RET_SUCCESS)
        printf("**WARNING**, buffer too small for ramoops bootargs.\n");
}
#endif

static void set_bootargs_ramload(int * pmem_size) {
    char * ramload_size = getenv("ubntramload.size");
    int size;

    ubnt_ramload_init(*pmem_size, 1);
    // Invalidate magic so can't be used again on reboot.
    ramload_state.ramload->magic = 0;

    *pmem_size = ramload_state.ramload;
    if (!ramload_size)
        return;

    size = simple_strtoul(ramload_size, NULL, 0);
    *pmem_size -= size;
    if(append_bootargs("ubntramload.size=0x%x ubntramload.addr=0x%x ubntramload.control=0x%x",
            size,
            *pmem_size,
            (unsigned int)ramload_state.ramload) != CMD_RET_SUCCESS)
        printf("**WARNING**, buffer too small for ramload bootargs.\n");
}

static int do_bootubnt(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
    int ret;
    int linux_mem = gd->ram_size;

    printf("ubnt boot ...\n");
    set_bootargs_uboot_version();
#ifdef UBNT_PERSISTENT_LOG
    set_bootargs_ramoops(&linux_mem);
#endif
    set_bootargs_ramload(&linux_mem);
    append_bootargs("mem=%luK", linux_mem >> 10);

#if defined(CONFIG_FIT)
    /*
     * set fdt_high parameter to all ones, so that u-boot will pass the
     * loaded in-place fdt address to kernel instead of relocating the fdt.
     */
#if 0 //jeffrey
    if (setenv_addr("fdt_high", (void *)CONFIG_IPQ_FDT_HIGH)
            != CMD_RET_SUCCESS) {
        printf("Cannot set fdt_high to %x to avoid relocation\n",
            CONFIG_IPQ_FDT_HIGH);
    }

    if(!ipq_smem_get_socinfo_version(&soc_version))
        debug("Soc version is = %x \n", SOCINFO_VERSION_MAJOR(soc_version));
    else
        printf("Cannot get socinfo, using defaults\n");
#endif
#endif /* CONFIG_FIT */

    ret = ubnt_app_start(--argc, &argv[1]);

    if (ret != 0) {
        printf("UBNT app error.\n");
        return CMD_RET_FAILURE;
    }

#if defined(CONFIG_FIT)
#if 0 //jeffrey
    snprintf((char *)dtb_config_name, sizeof(dtb_config_name),
        "#config@%d_%d", gboard_param->machid, SOCINFO_VERSION_MAJOR(soc_version));
#endif
#endif /* CONFIG_FIT */

    return load_kernel(p_share_data->kernel_addr);
}

U_BOOT_CMD(bootubnt, 5, 0, do_bootubnt,
       "bootubnt- ubnt boot from flash device\n",
       "bootubnt - Load image(s) and boots the kernel\n");

#define VERSION_LEN_MAX 128
/* The SSID is located after two 6-byte MAC addresses */
#define UBNT_SSID_OFFSET 12

uint8_t prepare_beacon(uint8_t *buf) {
    uint8_t *pkt = buf;
    uint8_t *len_p;

    // Lengths
    uint8_t total_len;
    uint8_t header_len;
    uint8_t body_len;


    // U-Boot Version
    char *version = U_BOOT_VERSION;
    uint8_t version_len = strlen(version);
    if (version_len > VERSION_LEN_MAX)
        version_len = VERSION_LEN_MAX;

    // SSID
    uint16_t *ubnt_ssid_p = (uint16_t *) spi_flash_addr(UBNT_PART_EEPROM_OFF + UBNT_SSID_OFFSET);

    /* header */
    *pkt++ = 0x03; // version
    *pkt++ = 0x07; // type (net rescue beacon)
    len_p = pkt++; // beacon size
    *pkt++ = 0x03; // bootloader version

    header_len = pkt - buf;

    /* uboot */
    *pkt++ = version_len;
    memcpy(pkt, version, version_len);
    pkt += version_len;

    /* board */
    memcpy(pkt, ubnt_ssid_p, sizeof(*ubnt_ssid_p));
    pkt += sizeof(*ubnt_ssid_p);

    total_len = pkt - buf;
    body_len = total_len - header_len;

    // Include size in beacon.
    *len_p = body_len;

    return total_len;
}
