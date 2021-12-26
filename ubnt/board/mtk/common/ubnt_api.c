// vim: ts=4:sw=4:expandtab
/* Copyright Ubiquti Networks Inc. 2017
 ** All Rights Reserved.
 */

#include <ubnt_config.h>
#include <common.h>
#include <exports.h>
#include <ubnt_types.h>
#include <board_types.h>
#include <types.h>
#include <config.h>
#include <ubnt_board.h>
#include <ubnt_mtd.h>
#include <image.h>
#include <ubnt_fw.h>

//#define DEBUG

extern ubnt_app_persistent_data_t ubnt_persistent_data;
extern unsigned short be16toh(unsigned short);
extern unsigned short le16toh(unsigned short);

static ubnt_app_share_data_t * p_share_data = &ubnt_persistent_data.share_data;
static ubnt_app_internal_data_t * p_internal_data = &ubnt_persistent_data.internal_data;
static char legacy_magic_number[4] = {0x27, 0x05, 0x19, 0x56};

enum ubnt_led_bitmask {
    LED_BITMASK_BLUE =      1 << 0,
    LED_BITMASK_WHITE =     1 << 1
};
enum ubnt_led_states {
    LED_STATE_ALL_OFF =  0,
    LED_STATE_BLUE_ON_ONLY = LED_BITMASK_BLUE,
    LED_STATE_WHITE_ON_ONLY = LED_BITMASK_WHITE,
    LED_STATE_ALL_ON = LED_BITMASK_BLUE | LED_BITMASK_WHITE,
    LED_STATE_END
};

enum ubnt_app_states {
    UAPP_STATE_UNKNOW =  0,
    UAPP_STATE_INIT_DONE = 1,
    UAPP_STATE_DO_URESCUE = 2,
    UAPP_STATE_READ_KERNEL_FIRST = 3,
    UAPP_STATE_READ_KERNEL_SECOND = 4,
    UAPP_STATE_BOOT = 5,
    UAPP_STATE_UPDATE_FIRMWARE = 6,
    UAPP_STATE_END
};

enum ubnt_app_flags {
    UAPP_FLAG_ALLOW_UPDATE_CRITICAL_PARTITION           =  0x1,
#ifdef SKIP_FW_VERSION_CHECK
    UAPP_FLAG_SKIP_FW_VERSION_CHECK                     =  0x2,
#endif
};

#define BOARDID_OFFSET_IN_EEPROM        0x0000c
#define VENDORID_OFFSET_IN_EEPROM       0x0000e
#define BOMREV_OFFSET_IN_EEPROM         0x00010

/* this structure comes from fwupdate.c */
#define UBNT_BS_MAGIC       0xA34DE82B

typedef struct ubnt_bootsel {
    unsigned int bootsel : 1;
    /* Due to backward compatability magic is placed in 2nd place */
    unsigned int magic;
} ubnt_bs_t;

#define UBNT_LED_STEPS_MAX 8
typedef struct ubnt_led_pattern {
    unsigned int patterns[UBNT_LED_STEPS_MAX];
} ubnt_led_pattern_t;

static ubnt_led_pattern_t led_pattern_tftp_waiting = {
    .patterns = {LED_STATE_ALL_OFF, LED_STATE_WHITE_ON_ONLY, LED_STATE_BLUE_ON_ONLY, LED_STATE_END, 0, 0, 0, 0}
};

static ubnt_led_pattern_t led_pattern_tftp_receiving = {
    .patterns = {LED_STATE_BLUE_ON_ONLY, LED_STATE_WHITE_ON_ONLY, LED_STATE_END, 0, 0, 0, 0, 0}
};

static ubnt_led_pattern_t led_pattern_firmware_updating = {
    .patterns = {LED_STATE_BLUE_ON_ONLY, LED_STATE_WHITE_ON_ONLY, LED_STATE_END, 0, 0, 0, 0, 0}
};

static ubnt_led_pattern_t led_pattern_firmware_reading = {
    .patterns = {LED_STATE_ALL_OFF, LED_STATE_WHITE_ON_ONLY, LED_STATE_END, 0, 0, 0, 0, 0}
};

void unifi_set_led(int pattern);

static void set_ubnt_app_state(int state) {

    if( state >= UAPP_STATE_END) {
        printf("State %d not supported.\n", state);
        return;
    }
#ifdef DEBUG
    printf("Change UBNT_APP State to %d.\n", state);
#endif

    p_internal_data->ubnt_app_state = state;
}

static int get_ubnt_app_state(void) {
    int state = p_internal_data->ubnt_app_state;

#ifdef DEBUG
    printf("Current UBNT_APP State %d.\n", state);
#endif
    return state;
}

static void update_led_state(void) {
    ubnt_led_pattern_t * p_led_pattern;
    const unsigned int pattern_size = sizeof(p_led_pattern->patterns)/sizeof(p_led_pattern->patterns[0]);

    p_led_pattern = (ubnt_led_pattern_t *)(p_internal_data->led_pattern);

    if( 0
        || (p_internal_data->led_current_step >= pattern_size)
        || (p_led_pattern->patterns[p_internal_data->led_current_step] == LED_STATE_END)
    ) {
        p_internal_data->led_current_step = 0;
    }
    unifi_set_led(p_led_pattern->patterns[p_internal_data->led_current_step]);
    p_internal_data->led_current_step ++;
}

#ifdef SKIP_FW_VERSION_CHECK
int skip_fw_version_check() {
    return (p_internal_data->flags & UAPP_FLAG_SKIP_FW_VERSION_CHECK) ? 1 : 0;
}
#endif

int get_boardid(unsigned short * boardid) {
    int ret = 0;
    unsigned int data_addr;
    const unsigned short * p_boardid;
    /* MTK board has memory mapped to access first 4MB of SPI flash.
     * As long as EEPROM is contained below 4MB mark this logic will work.
     * Otherwise the EEPROM needs to be cached to RAM*/
    if (!get_mtd_params(UBNT_PART_EEPROM_NAME, &data_addr, NULL, NULL, NULL)) {
        printf("Could not find partition %s\n", UBNT_PART_EEPROM_NAME);
        return -1;
    }

    data_addr += BOARDID_OFFSET_IN_EEPROM;
    p_boardid = (const unsigned short *)data_addr;
    *boardid = ntohs(*p_boardid);
#ifdef DEBUG
    printf("eeprom data addr: 0x%08x, boardid: 0x%x, ret: %d\n", data_addr, *boardid, ret);
#endif

    return ret;
}

int get_vendorid(unsigned short * vendorid) {
    int ret = 0;
    unsigned int data_addr;
    const unsigned short * p_vendorid;
    /* MTK board has memory mapped to access first 4MB of SPI flash.
     * As long as EEPROM is contained below 4MB mark this logic will work.
     * Otherwise the EEPROM needs to be cached to RAM*/
    if (!get_mtd_params(UBNT_PART_EEPROM_NAME, &data_addr, NULL, NULL, NULL)) {
        printf("Could not find partition %s\n", UBNT_PART_EEPROM_NAME);
        return -1;
    }

    data_addr += VENDORID_OFFSET_IN_EEPROM;
    p_vendorid = (const unsigned short *)data_addr;
    *vendorid = ntohs(*p_vendorid);
#ifdef DEBUG
    printf("eeprom data addr: 0x%08x, vendorid: 0x%x, ret: %d\n", data_addr, *vendorid, ret);
#endif

    return ret;
}

int get_bomrev(unsigned int * bomrev) {
    int ret = 0;
    unsigned int data_addr;
    const unsigned int * p_bomrev;
    /* MTK board has memory mapped to access first 4MB of SPI flash.
     * As long as EEPROM is contained below 4MB mark this logic will work.
     * Otherwise the EEPROM needs to be cached to RAM*/
    if (!get_mtd_params(UBNT_PART_EEPROM_NAME, &data_addr, NULL, NULL, NULL)) {
        printf("Could not find partition %s\n", UBNT_PART_EEPROM_NAME);
        return -1;
    }

    data_addr += BOMREV_OFFSET_IN_EEPROM;
    p_bomrev = (const unsigned int *)data_addr;
    *bomrev = ntohl(*p_bomrev);
#ifdef DEBUG
    printf("eeprom data addr: 0x%08x, bomrev: 0x%x, ret: %d\n", data_addr, *bomrev, ret);
#endif

    return ret;
}

int get_boardrevision(int * boardrevision) {
    int ret = 0;
    unsigned int bomrev = 0xffffffff;

    ret = get_bomrev(&bomrev);
    if (ret) {
        printf("WARNING: Cannot get BOM version\n");
    } else if (bomrev == 0xffffffff) {
        *boardrevision = -1;
    } else {
        *boardrevision = bomrev & 0xff;
    }

#ifdef DEBUG
    printf("boardrevision %d, bomrev: 0x%x, ret: %d\n", *boardrevision, bomrev, ret);
#endif

    return ret;
}

static int get_boot_select(int * boot_sel) {
    int ret = 0;
    unsigned int data_addr;
    const ubnt_bs_t * p_boot_select;
    *boot_sel = 0;
    if (!get_mtd_params(UBNT_PART_BOOTSELECT_NAME, &data_addr, NULL, NULL, NULL)) {
        printf("Could not find partition %s\n", UBNT_PART_BOOTSELECT_NAME);
        return -1;
    }
    p_boot_select = (const ubnt_bs_t *)data_addr;
#ifdef DEBUG
    printf("bootselect data addr: 0x%08x, magic: 0x%08x, boot_select: 0x%08x\n",
           data_addr, p_boot_select->magic, p_boot_select->bootsel);
#endif
    if (p_boot_select->magic != UBNT_BS_MAGIC) {
        printf("WARNING:Boot selection logic magic mismatch!\n");
        /* Return kernel 0  partition */
        *boot_sel = 0;
        ret = -1;
    } else {
        *boot_sel = p_boot_select->bootsel;
    }
#ifdef DEBUG
    printf("boot_select: %d, ret: %d\n", *boot_sel, ret);
#endif

    return ret;
}

static void set_do_urescue_flag(void) {
    /* after urescue,
     * 1) kernel 0 will be updated
     * 2) booselect partition will be erased
     */
    setenv("do_urescue", "TRUE");
    p_internal_data->led_pattern = (unsigned int)(&led_pattern_tftp_waiting);
    p_internal_data->led_current_step = 0;
}

static int
fix_eeprom_crc(void *base, void* end, int is_big_endian) {
    unsigned short sum = 0, data, *pHalf, *pSum;
    unsigned short lengthStruct, i;

	lengthStruct = is_big_endian ? be16toh(*(uint16_t*)base) : le16toh(*(uint16_t*)base);

    if ((((unsigned int)base+lengthStruct) > ((unsigned int)end))
            || (lengthStruct == 0)) {
        return 0;
    }

    /* calc */
    pHalf = (unsigned short *)base;

    /* clear crc */
    pSum = pHalf + 1;
    *pSum = 0x0000;

    for (i = 0; i < lengthStruct/2; i++) {
        data = *pHalf++;
        sum ^= is_big_endian ? be16toh(data) : le16toh(data);
    }

    sum ^= 0xffff;
    *pSum = is_big_endian ? be16toh(sum) : le16toh(sum);

    return 1;
}

#ifdef CONFIG_FLASH_WP
int
ubnt_enable_protection(void) {

    const ubnt_bdinfo_t *bd = board_identify(NULL);

    if (!bd)
        return -1;

    /* If GPIO0 is used for reset, then use BP only Software Protection*/

    if (!bd->reset_button_gpio) {
        ubnt_flash_enable_protection(0);
    }
    else {
        /* Otherwise use, HW (WPEN/GPIO) protection */
        ubnt_flash_enable_protection(1);
    }

    return 0;
}

int
ubnt_disable_protection(void) {
    const ubnt_bdinfo_t *bd = board_identify(NULL);

    if (!bd)
        return -1;

    /* If GPIO0 is used for reset, then use BP only Software Protection*/

    if (!bd->reset_button_gpio) {
        ubnt_flash_disable_protection(0);
    }
    else {
        /* Otherwise use, HW (WPEN/GPIO) protection */
        ubnt_flash_disable_protection(1);
    }

    return 0;
}
#else
int
ubnt_enable_protection(void) {
    return 0;
}

int
ubnt_disable_protection(void) {
    return 0;
}
#endif  // #ifdef CONFIG_FLASH_WP

    /* Initializes this application */

ubnt_exec_t
ubnt_uappinit(int argc, char *argv[]) {
    int kernel_index;
    const ubnt_bdinfo_t *bd = board_identify(NULL);
    ubnt_exec_t res;
    int i;

    res.ret = 0;

    if (!bd)
        goto ubnt_err;
#if 0
    mtdparts_init();
    flash_init();
    ubnt_env_init();
#ifdef QCA_11AC
    ubnt_bootsel_init();
#endif
    /* By default flash protection should be on during bootup */
    if (ubnt_enable_protection())
        goto ubnt_err;
#endif

    ubnt_env_init();

    get_boot_select(&kernel_index);
    /* Set all update addresses to invalid */
    p_share_data->kernel_index = kernel_index;
    memset(&p_share_data->update, UBNT_INVALID_ADDR, sizeof(p_share_data->update));

    p_internal_data->led_pattern = 0;
    p_internal_data->led_current_step = 0;
    p_internal_data->flags = UAPP_FLAG_ALLOW_UPDATE_CRITICAL_PARTITION;
    set_ubnt_app_state(UAPP_STATE_INIT_DONE);

    for(i = 1; i < argc; i++) {
        if (!(strcmp(argv[i], "-f"))) {
#ifdef SKIP_FW_VERSION_CHECK
#ifdef DEBUG
            printf("Allow upgrading to non-tagged fw version.\n");
#endif
            p_internal_data->flags |= UAPP_FLAG_SKIP_FW_VERSION_CHECK;
#endif
        }
    }

    printf("UBNT application initialized \n");
    return res;

ubnt_err:
    res.ret = 1;
    return res;
}

#ifdef CONFIG_FLASH_WP
ubnt_exec_t
ubnt_usetprotect(int argc, char *argv[]) {
    int use_wp_hw = 1;
    int i = 1;
    ubnt_exec_t res;

    res.ret = 0;

    if (argc < 2){
        ubnt_flash_print_protection_status();
        return res;
    }

    if (argc > 3){
        goto ubnt_err;
    }
    if (argc == 3) {
        if (0
            || strcmp(argv[i], "HPM") == 0
            || strcmp(argv[i], "HW") == 0
            || strcmp(argv[i], "H") == 0
            || strcmp(argv[i], "hpm") == 0
            || strcmp(argv[i], "hw") == 0
            || strcmp(argv[i], "h") == 0
            )
        {
            use_wp_hw = 1;
        } else if (0
            || strcmp(argv[i], "SPM") == 0
            || strcmp(argv[i], "SW") == 0
            || strcmp(argv[i], "S") == 0
            || strcmp(argv[i], "spm") == 0
            || strcmp(argv[i], "sw") == 0
            || strcmp(argv[i], "s") == 0
            )
        {
            use_wp_hw = 0;
        } else {
            goto ubnt_err;
        }
        i++;
    }
    if (strcmp(argv[i], "on") == 0) {
        ubnt_flash_enable_protection(use_wp_hw);
    } else if (strcmp(argv[i], "off") == 0) {
        ubnt_flash_disable_protection(use_wp_hw);
    } else {
        goto ubnt_err;
    }
    return res;

ubnt_err:
    res.ret = 1;
    return res;
}
#else
ubnt_exec_t
ubnt_usetprotect(int argc, char *argv[]) {
    ubnt_exec_t res;

    res.ret = 0;

    return res;
}
#endif  // #ifdef CONFIG_FLASH_WP

#ifdef QCA_11AC
static int
_do_clear_ubootenv(void) {
#if 0
    flash_uinfo_t* flinfo = &flash_info[0];
    int sa=0, sz=0, ssect=0, esect=0;

    if (!get_mtd_params("u-boot-env", &sa, &sz, &ssect, &esect)) {
        printf("Invalid mtd name\n");
        return -1;
    }

#ifdef DEBUG
    printf ("Erasing sector %d..%d, sector size=%d\n", ssect , esect, flinfo->size/flinfo->sector_count);
#endif

    flash_erase(flinfo, ssect, esect);
#endif
    return 0;
}

#else
static int
_do_clear_ubootenv(void) {
#if 0
    flash_uinfo_t* flinfo = &flash_info[0];
    int sector_size = flinfo->size / flinfo->sector_count;
    int num_sector, s_first;

    num_sector = CFG_ENV_SIZE / sector_size;
    s_first = (CFG_ENV_ADDR - flinfo->base[0]) / sector_size;

#ifdef DEBUG
    printf ("Erasing sector %d..%d\n", s_first , s_first + num_sector - 1);
#endif
    flash_erase(flinfo, s_first, s_first + num_sector - 1);
#endif
    return 0;
}
#endif


static int
_do_clearcfg(void) {
#if 0
    flash_uinfo_t* flinfo = &flash_info[0];
    int sector_size = flinfo->size / flinfo->sector_count;
    //unsigned long cfg_addr = BOARDCAL - UBNT_CFG_PART_SIZE;
    int num_sector, s_first;

    num_sector = UBNT_CFG_PART_SIZE / sector_size;
    s_first = CAL_SECTOR - num_sector;

#ifdef DEBUG
    printf ("Erasing sector %d..%d\n", s_first , s_first + num_sector - 1);
#endif
    flash_erase(flinfo, s_first, s_first + num_sector - 1);
#endif
    return 0;
}

ubnt_exec_t
ubnt_uclearcfg(int argc, char *argv[]) {
    ubnt_exec_t res;
    res.ret = _do_clearcfg();
    return res;
}

ubnt_exec_t
ubnt_uclearenv(int argc, char *argv[]) {
    ubnt_exec_t res;

    res.ret = _do_clear_ubootenv();

    return res;
}


/* GPIO API */
// jeffrey TODO: move to another place
#define RSUCCESS        0
#define ERACCESS        1
#define ERINVAL         2
#define ERWRAPPER       3
#define GPIO_BASE (0x10211000)
#define GPIO_WR32(addr, data)   ((*(volatile unsigned long*)(addr)) = (unsigned long)(data))
#define GPIO_RD32(addr)         (*(volatile unsigned long * const)(addr))
#define GPIO_SET_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) = (unsigned long)(BIT))
#define GPIO_CLR_BITS(BIT,REG)   ((*(volatile unsigned long*)(REG)) &= ~((unsigned long)(BIT)))

struct mtk_pin_info {
    unsigned int pin;
    unsigned int offset;
    unsigned char bit;
    unsigned char width;
};

#define MTK_PIN_INFO(_pin, _offset, _bit, _width)\
{\
    .pin = _pin,\
    .offset = _offset,\
    .bit = _bit, \
    .width = _width \
}

static struct mtk_pin_info *mtk_pinctrl_get_gpio_array(int pin, int size,
    const struct mtk_pin_info pArray[])
{
    unsigned long i = 0;

    for (i = 0; i < size; i++) {
        if (pin == pArray[i].pin)
            return &pArray[i];
    }
    printf("cant found pin %d for size %d\n", pin, size);
    return NULL;
}

static const struct mtk_pin_info mtk_pin_info_mode[] = {
    MTK_PIN_INFO(0, 0x320, 16, 4),
    MTK_PIN_INFO(1, 0x3A0, 16, 4),
    MTK_PIN_INFO(2, 0x3A0, 20, 4),
    MTK_PIN_INFO(3, 0x3A0, 24, 4),
    MTK_PIN_INFO(4, 0x3A0, 28, 4),
    MTK_PIN_INFO(5, 0x320, 0, 4),
    MTK_PIN_INFO(6, 0x300, 4, 4),
    MTK_PIN_INFO(7, 0x300, 4, 4),
    MTK_PIN_INFO(8, 0x350, 20, 4),
    MTK_PIN_INFO(9, 0x350, 24, 4),
    MTK_PIN_INFO(10, 0x300, 8, 4),
    MTK_PIN_INFO(11, 0x300, 8, 4),
    MTK_PIN_INFO(12, 0x300, 8, 4),
    MTK_PIN_INFO(13, 0x300, 8, 4),
    MTK_PIN_INFO(14, 0x320, 4, 4),
    MTK_PIN_INFO(15, 0x320, 8, 4),
    MTK_PIN_INFO(16, 0x320, 20, 4),
    MTK_PIN_INFO(17, 0x320, 24, 4),
    MTK_PIN_INFO(18, 0x310, 16, 4),
    MTK_PIN_INFO(19, 0x310, 20, 4),
    MTK_PIN_INFO(20, 0x310, 24, 4),
    MTK_PIN_INFO(21, 0x310, 28, 4),
    MTK_PIN_INFO(22, 0x380, 16, 4),
    MTK_PIN_INFO(23, 0x300, 24, 4),
    MTK_PIN_INFO(24, 0x300, 24, 4),
    MTK_PIN_INFO(25, 0x300, 12, 4),
    MTK_PIN_INFO(26, 0x300, 12, 4),
    MTK_PIN_INFO(27, 0x300, 12, 4),
    MTK_PIN_INFO(28, 0x300, 12, 4),
    MTK_PIN_INFO(29, 0x300, 12, 4),
    MTK_PIN_INFO(30, 0x300, 12, 4),
    MTK_PIN_INFO(31, 0x300, 12, 4),
    MTK_PIN_INFO(32, 0x300, 12, 4),
    MTK_PIN_INFO(33, 0x300, 12, 4),
    MTK_PIN_INFO(34, 0x300, 12, 4),
    MTK_PIN_INFO(35, 0x300, 12, 4),
    MTK_PIN_INFO(36, 0x300, 12, 4),
    MTK_PIN_INFO(37, 0x300, 20, 4),
    MTK_PIN_INFO(38, 0x300, 20, 4),
    MTK_PIN_INFO(39, 0x300, 20, 4),
    MTK_PIN_INFO(40, 0x300, 20, 4),
    MTK_PIN_INFO(41, 0x300, 20, 4),
    MTK_PIN_INFO(42, 0x300, 20, 4),
    MTK_PIN_INFO(43, 0x300, 20, 4),
    MTK_PIN_INFO(44, 0x300, 20, 4),
    MTK_PIN_INFO(45, 0x300, 20, 4),
    MTK_PIN_INFO(46, 0x300, 20, 4),
    MTK_PIN_INFO(47, 0x300, 20, 4),
    MTK_PIN_INFO(48, 0x300, 20, 4),
    MTK_PIN_INFO(49, 0x300, 20, 4),
    MTK_PIN_INFO(50, 0x300, 20, 4),
    MTK_PIN_INFO(51, 0x330, 4, 4),
    MTK_PIN_INFO(52, 0x330, 8, 4),
    MTK_PIN_INFO(53, 0x330, 12, 4),
    MTK_PIN_INFO(54, 0x330, 16, 4),
    MTK_PIN_INFO(55, 0x330, 20, 4),
    MTK_PIN_INFO(56, 0x330, 24, 4),
    MTK_PIN_INFO(57, 0x330, 28, 4),
    MTK_PIN_INFO(58, 0x340, 0, 4),
    MTK_PIN_INFO(59, 0x340, 4, 4),
    MTK_PIN_INFO(60, 0x340, 8, 4),
    MTK_PIN_INFO(61, 0x340, 12, 4),
    MTK_PIN_INFO(62, 0x340, 16, 4),
    MTK_PIN_INFO(63, 0x340, 20, 4),
    MTK_PIN_INFO(64, 0x340, 24, 4),
    MTK_PIN_INFO(65, 0x340, 28, 4),
    MTK_PIN_INFO(66, 0x350, 0, 4),
    MTK_PIN_INFO(67, 0x350, 4, 4),
    MTK_PIN_INFO(68, 0x350, 8, 4),
    MTK_PIN_INFO(69, 0x350, 12, 4),
    MTK_PIN_INFO(70, 0x350, 16, 4),
    MTK_PIN_INFO(71, 0x300, 16, 4),
    MTK_PIN_INFO(72, 0x300, 16, 4),
    MTK_PIN_INFO(73, 0x310, 0, 4),
    MTK_PIN_INFO(74, 0x310, 4, 4),
    MTK_PIN_INFO(75, 0x310, 8, 4),
    MTK_PIN_INFO(76, 0x310, 12, 4),
    MTK_PIN_INFO(77, 0x320, 28, 4),
    MTK_PIN_INFO(78, 0x320, 12, 4),
    MTK_PIN_INFO(79, 0x3A0, 0, 4),
    MTK_PIN_INFO(80, 0x3A0, 4, 4),
    MTK_PIN_INFO(81, 0x3A0, 8, 4),
    MTK_PIN_INFO(82, 0x3A0, 12, 4),
    MTK_PIN_INFO(83, 0x350, 28, 4),
    MTK_PIN_INFO(84, 0x330, 0, 4),
    MTK_PIN_INFO(85, 0x360, 4, 4),
    MTK_PIN_INFO(86, 0x360, 8, 4),
    MTK_PIN_INFO(87, 0x360, 12, 4),
    MTK_PIN_INFO(88, 0x360, 16, 4),
    MTK_PIN_INFO(89, 0x360, 20, 4),
    MTK_PIN_INFO(90, 0x360, 24, 4),
    MTK_PIN_INFO(91, 0x390, 16, 4),
    MTK_PIN_INFO(92, 0x390, 20, 4),
    MTK_PIN_INFO(93, 0x390, 24, 4),
    MTK_PIN_INFO(94, 0x390, 28, 4),
    MTK_PIN_INFO(95, 0x380, 20, 4),
    MTK_PIN_INFO(96, 0x380, 24, 4),
    MTK_PIN_INFO(97, 0x380, 28, 4),
    MTK_PIN_INFO(98, 0x390, 0, 4),
    MTK_PIN_INFO(99, 0x390, 4, 4),
    MTK_PIN_INFO(100, 0x390, 8, 4),
    MTK_PIN_INFO(101, 0x390, 12, 4),
    MTK_PIN_INFO(102, 0x360, 0, 4),
};

static const struct mtk_pin_info mtk_pin_info_dir[] = {
    MTK_PIN_INFO(0, 0x000, 0, 1),
    MTK_PIN_INFO(1, 0x000, 1, 1),
    MTK_PIN_INFO(2, 0x000, 2, 1),
    MTK_PIN_INFO(3, 0x000, 3, 1),
    MTK_PIN_INFO(4, 0x000, 4, 1),
    MTK_PIN_INFO(5, 0x000, 5, 1),
    MTK_PIN_INFO(6, 0x000, 6, 1),
    MTK_PIN_INFO(7, 0x000, 7, 1),
    MTK_PIN_INFO(8, 0x000, 8, 1),
    MTK_PIN_INFO(9, 0x000, 9, 1),
    MTK_PIN_INFO(10, 0x000, 10, 1),
    MTK_PIN_INFO(11, 0x000, 11, 1),
    MTK_PIN_INFO(12, 0x000, 12, 1),
    MTK_PIN_INFO(13, 0x000, 13, 1),
    MTK_PIN_INFO(14, 0x000, 14, 1),
    MTK_PIN_INFO(15, 0x000, 15, 1),
    MTK_PIN_INFO(16, 0x000, 16, 1),
    MTK_PIN_INFO(17, 0x000, 17, 1),
    MTK_PIN_INFO(18, 0x000, 18, 1),
    MTK_PIN_INFO(19, 0x000, 19, 1),
    MTK_PIN_INFO(20, 0x000, 20, 1),
    MTK_PIN_INFO(21, 0x000, 21, 1),
    MTK_PIN_INFO(22, 0x000, 22, 1),
    MTK_PIN_INFO(23, 0x000, 23, 1),
    MTK_PIN_INFO(24, 0x000, 24, 1),
    MTK_PIN_INFO(25, 0x000, 25, 1),
    MTK_PIN_INFO(26, 0x000, 26, 1),
    MTK_PIN_INFO(27, 0x000, 27, 1),
    MTK_PIN_INFO(28, 0x000, 28, 1),
    MTK_PIN_INFO(29, 0x000, 29, 1),
    MTK_PIN_INFO(30, 0x000, 30, 1),
    MTK_PIN_INFO(31, 0x000, 31, 1),
    MTK_PIN_INFO(32, 0x010, 0, 1),
    MTK_PIN_INFO(33, 0x010, 1, 1),
    MTK_PIN_INFO(34, 0x010, 2, 1),
    MTK_PIN_INFO(35, 0x010, 3, 1),
    MTK_PIN_INFO(36, 0x010, 4, 1),
    MTK_PIN_INFO(37, 0x010, 5, 1),
    MTK_PIN_INFO(38, 0x010, 6, 1),
    MTK_PIN_INFO(39, 0x010, 7, 1),
    MTK_PIN_INFO(40, 0x010, 8, 1),
    MTK_PIN_INFO(41, 0x010, 9, 1),
    MTK_PIN_INFO(42, 0x010, 10, 1),
    MTK_PIN_INFO(43, 0x010, 11, 1),
    MTK_PIN_INFO(44, 0x010, 12, 1),
    MTK_PIN_INFO(45, 0x010, 13, 1),
    MTK_PIN_INFO(46, 0x010, 14, 1),
    MTK_PIN_INFO(47, 0x010, 15, 1),
    MTK_PIN_INFO(48, 0x010, 16, 1),
    MTK_PIN_INFO(49, 0x010, 17, 1),
    MTK_PIN_INFO(50, 0x010, 18, 1),
    MTK_PIN_INFO(51, 0x010, 19, 1),
    MTK_PIN_INFO(52, 0x010, 20, 1),
    MTK_PIN_INFO(53, 0x010, 21, 1),
    MTK_PIN_INFO(54, 0x010, 22, 1),
    MTK_PIN_INFO(55, 0x010, 23, 1),
    MTK_PIN_INFO(56, 0x010, 24, 1),
    MTK_PIN_INFO(57, 0x010, 25, 1),
    MTK_PIN_INFO(58, 0x010, 26, 1),
    MTK_PIN_INFO(59, 0x010, 27, 1),
    MTK_PIN_INFO(60, 0x010, 28, 1),
    MTK_PIN_INFO(61, 0x010, 29, 1),
    MTK_PIN_INFO(62, 0x010, 30, 1),
    MTK_PIN_INFO(63, 0x010, 31, 1),
    MTK_PIN_INFO(64, 0x020, 0, 1),
    MTK_PIN_INFO(65, 0x020, 1, 1),
    MTK_PIN_INFO(66, 0x020, 2, 1),
    MTK_PIN_INFO(67, 0x020, 3, 1),
    MTK_PIN_INFO(68, 0x020, 4, 1),
    MTK_PIN_INFO(69, 0x020, 5, 1),
    MTK_PIN_INFO(70, 0x020, 6, 1),
    MTK_PIN_INFO(71, 0x020, 7, 1),
    MTK_PIN_INFO(72, 0x020, 8, 1),
    MTK_PIN_INFO(73, 0x020, 9, 1),
    MTK_PIN_INFO(74, 0x020, 10, 1),
    MTK_PIN_INFO(75, 0x020, 11, 1),
    MTK_PIN_INFO(76, 0x020, 12, 1),
    MTK_PIN_INFO(77, 0x020, 13, 1),
    MTK_PIN_INFO(78, 0x020, 14, 1),
    MTK_PIN_INFO(79, 0x020, 15, 1),
    MTK_PIN_INFO(80, 0x020, 16, 1),
    MTK_PIN_INFO(81, 0x020, 17, 1),
    MTK_PIN_INFO(82, 0x020, 18, 1),
    MTK_PIN_INFO(83, 0x020, 19, 1),
    MTK_PIN_INFO(84, 0x020, 20, 1),
    MTK_PIN_INFO(85, 0x020, 21, 1),
    MTK_PIN_INFO(86, 0x020, 22, 1),
    MTK_PIN_INFO(87, 0x020, 23, 1),
    MTK_PIN_INFO(88, 0x020, 24, 1),
    MTK_PIN_INFO(89, 0x020, 25, 1),
    MTK_PIN_INFO(90, 0x020, 26, 1),
    MTK_PIN_INFO(91, 0x020, 27, 1),
    MTK_PIN_INFO(92, 0x020, 28, 1),
    MTK_PIN_INFO(93, 0x020, 29, 1),
    MTK_PIN_INFO(94, 0x020, 30, 1),
    MTK_PIN_INFO(95, 0x020, 31, 1),
    MTK_PIN_INFO(96, 0x030, 0, 1),
    MTK_PIN_INFO(97, 0x030, 1, 1),
    MTK_PIN_INFO(98, 0x030, 2, 1),
    MTK_PIN_INFO(99, 0x030, 3, 1),
    MTK_PIN_INFO(100, 0x030, 4, 1),
    MTK_PIN_INFO(101, 0x030, 5, 1),
    MTK_PIN_INFO(102, 0x030, 6, 1),
};

static const struct mtk_pin_info mtk_pin_info_datain[] = {
    MTK_PIN_INFO(0, 0x200, 0, 1),
    MTK_PIN_INFO(1, 0x200, 1, 1),
    MTK_PIN_INFO(2, 0x200, 2, 1),
    MTK_PIN_INFO(3, 0x200, 3, 1),
    MTK_PIN_INFO(4, 0x200, 4, 1),
    MTK_PIN_INFO(5, 0x200, 5, 1),
    MTK_PIN_INFO(6, 0x200, 6, 1),
    MTK_PIN_INFO(7, 0x200, 7, 1),
    MTK_PIN_INFO(8, 0x200, 8, 1),
    MTK_PIN_INFO(9, 0x200, 9, 1),
    MTK_PIN_INFO(10, 0x200, 10, 1),
    MTK_PIN_INFO(11, 0x200, 11, 1),
    MTK_PIN_INFO(12, 0x200, 12, 1),
    MTK_PIN_INFO(13, 0x200, 13, 1),
    MTK_PIN_INFO(14, 0x200, 14, 1),
    MTK_PIN_INFO(15, 0x200, 15, 1),
    MTK_PIN_INFO(16, 0x200, 16, 1),
    MTK_PIN_INFO(17, 0x200, 17, 1),
    MTK_PIN_INFO(18, 0x200, 18, 1),
    MTK_PIN_INFO(19, 0x200, 19, 1),
    MTK_PIN_INFO(20, 0x200, 20, 1),
    MTK_PIN_INFO(21, 0x200, 21, 1),
    MTK_PIN_INFO(22, 0x200, 22, 1),
    MTK_PIN_INFO(23, 0x200, 23, 1),
    MTK_PIN_INFO(24, 0x200, 24, 1),
    MTK_PIN_INFO(25, 0x200, 25, 1),
    MTK_PIN_INFO(26, 0x200, 26, 1),
    MTK_PIN_INFO(27, 0x200, 27, 1),
    MTK_PIN_INFO(28, 0x200, 28, 1),
    MTK_PIN_INFO(29, 0x200, 29, 1),
    MTK_PIN_INFO(30, 0x200, 30, 1),
    MTK_PIN_INFO(31, 0x200, 31, 1),
    MTK_PIN_INFO(32, 0x210, 0, 1),
    MTK_PIN_INFO(33, 0x210, 1, 1),
    MTK_PIN_INFO(34, 0x210, 2, 1),
    MTK_PIN_INFO(35, 0x210, 3, 1),
    MTK_PIN_INFO(36, 0x210, 4, 1),
    MTK_PIN_INFO(37, 0x210, 5, 1),
    MTK_PIN_INFO(38, 0x210, 6, 1),
    MTK_PIN_INFO(39, 0x210, 7, 1),
    MTK_PIN_INFO(40, 0x210, 8, 1),
    MTK_PIN_INFO(41, 0x210, 9, 1),
    MTK_PIN_INFO(42, 0x210, 10, 1),
    MTK_PIN_INFO(43, 0x210, 11, 1),
    MTK_PIN_INFO(44, 0x210, 12, 1),
    MTK_PIN_INFO(45, 0x210, 13, 1),
    MTK_PIN_INFO(46, 0x210, 14, 1),
    MTK_PIN_INFO(47, 0x210, 15, 1),
    MTK_PIN_INFO(48, 0x210, 16, 1),
    MTK_PIN_INFO(49, 0x210, 17, 1),
    MTK_PIN_INFO(50, 0x210, 18, 1),
    MTK_PIN_INFO(51, 0x210, 19, 1),
    MTK_PIN_INFO(52, 0x210, 20, 1),
    MTK_PIN_INFO(53, 0x210, 21, 1),
    MTK_PIN_INFO(54, 0x210, 22, 1),
    MTK_PIN_INFO(55, 0x210, 23, 1),
    MTK_PIN_INFO(56, 0x210, 24, 1),
    MTK_PIN_INFO(57, 0x210, 25, 1),
    MTK_PIN_INFO(58, 0x210, 26, 1),
    MTK_PIN_INFO(59, 0x210, 27, 1),
    MTK_PIN_INFO(60, 0x210, 28, 1),
    MTK_PIN_INFO(61, 0x210, 29, 1),
    MTK_PIN_INFO(62, 0x210, 30, 1),
    MTK_PIN_INFO(63, 0x210, 31, 1),
    MTK_PIN_INFO(64, 0x220, 0, 1),
    MTK_PIN_INFO(65, 0x220, 1, 1),
    MTK_PIN_INFO(66, 0x220, 2, 1),
    MTK_PIN_INFO(67, 0x220, 3, 1),
    MTK_PIN_INFO(68, 0x220, 4, 1),
    MTK_PIN_INFO(69, 0x220, 5, 1),
    MTK_PIN_INFO(70, 0x220, 6, 1),
    MTK_PIN_INFO(71, 0x220, 7, 1),
    MTK_PIN_INFO(72, 0x220, 8, 1),
    MTK_PIN_INFO(73, 0x220, 9, 1),
    MTK_PIN_INFO(74, 0x220, 10, 1),
    MTK_PIN_INFO(75, 0x220, 11, 1),
    MTK_PIN_INFO(76, 0x220, 12, 1),
    MTK_PIN_INFO(77, 0x220, 13, 1),
    MTK_PIN_INFO(78, 0x220, 14, 1),
    MTK_PIN_INFO(79, 0x220, 15, 1),
    MTK_PIN_INFO(80, 0x220, 16, 1),
    MTK_PIN_INFO(81, 0x220, 17, 1),
    MTK_PIN_INFO(82, 0x220, 18, 1),
    MTK_PIN_INFO(83, 0x220, 19, 1),
    MTK_PIN_INFO(84, 0x220, 20, 1),
    MTK_PIN_INFO(85, 0x220, 21, 1),
    MTK_PIN_INFO(86, 0x220, 22, 1),
    MTK_PIN_INFO(87, 0x220, 23, 1),
    MTK_PIN_INFO(88, 0x220, 24, 1),
    MTK_PIN_INFO(89, 0x220, 25, 1),
    MTK_PIN_INFO(90, 0x220, 26, 1),
    MTK_PIN_INFO(91, 0x220, 27, 1),
    MTK_PIN_INFO(92, 0x220, 28, 1),
    MTK_PIN_INFO(93, 0x220, 29, 1),
    MTK_PIN_INFO(94, 0x220, 30, 1),
    MTK_PIN_INFO(95, 0x220, 31, 1),
    MTK_PIN_INFO(96, 0x230, 0, 1),
    MTK_PIN_INFO(97, 0x230, 1, 1),
    MTK_PIN_INFO(98, 0x230, 2, 1),
    MTK_PIN_INFO(99, 0x230, 3, 1),
    MTK_PIN_INFO(100, 0x230, 4, 1),
    MTK_PIN_INFO(101, 0x230, 5, 1),
    MTK_PIN_INFO(102, 0x230, 6, 1),
};

static const struct mtk_pin_info mtk_pin_info_dataout[] = {
    MTK_PIN_INFO(0, 0x100, 0, 1),
    MTK_PIN_INFO(1, 0x100, 1, 1),
    MTK_PIN_INFO(2, 0x100, 2, 1),
    MTK_PIN_INFO(3, 0x100, 3, 1),
    MTK_PIN_INFO(4, 0x100, 4, 1),
    MTK_PIN_INFO(5, 0x100, 5, 1),
    MTK_PIN_INFO(6, 0x100, 6, 1),
    MTK_PIN_INFO(7, 0x100, 7, 1),
    MTK_PIN_INFO(8, 0x100, 8, 1),
    MTK_PIN_INFO(9, 0x100, 9, 1),
    MTK_PIN_INFO(10, 0x100, 10, 1),
    MTK_PIN_INFO(11, 0x100, 11, 1),
    MTK_PIN_INFO(12, 0x100, 12, 1),
    MTK_PIN_INFO(13, 0x100, 13, 1),
    MTK_PIN_INFO(14, 0x100, 14, 1),
    MTK_PIN_INFO(15, 0x100, 15, 1),
    MTK_PIN_INFO(16, 0x100, 16, 1),
    MTK_PIN_INFO(17, 0x100, 17, 1),
    MTK_PIN_INFO(18, 0x100, 18, 1),
    MTK_PIN_INFO(19, 0x100, 19, 1),
    MTK_PIN_INFO(20, 0x100, 20, 1),
    MTK_PIN_INFO(21, 0x100, 21, 1),
    MTK_PIN_INFO(22, 0x100, 22, 1),
    MTK_PIN_INFO(23, 0x100, 23, 1),
    MTK_PIN_INFO(24, 0x100, 24, 1),
    MTK_PIN_INFO(25, 0x100, 25, 1),
    MTK_PIN_INFO(26, 0x100, 26, 1),
    MTK_PIN_INFO(27, 0x100, 27, 1),
    MTK_PIN_INFO(28, 0x100, 28, 1),
    MTK_PIN_INFO(29, 0x100, 29, 1),
    MTK_PIN_INFO(30, 0x100, 30, 1),
    MTK_PIN_INFO(31, 0x100, 31, 1),
    MTK_PIN_INFO(32, 0x110, 0, 1),
    MTK_PIN_INFO(33, 0x110, 1, 1),
    MTK_PIN_INFO(34, 0x110, 2, 1),
    MTK_PIN_INFO(35, 0x110, 3, 1),
    MTK_PIN_INFO(36, 0x110, 4, 1),
    MTK_PIN_INFO(37, 0x110, 5, 1),
    MTK_PIN_INFO(38, 0x110, 6, 1),
    MTK_PIN_INFO(39, 0x110, 7, 1),
    MTK_PIN_INFO(40, 0x110, 8, 1),
    MTK_PIN_INFO(41, 0x110, 9, 1),
    MTK_PIN_INFO(42, 0x110, 10, 1),
    MTK_PIN_INFO(43, 0x110, 11, 1),
    MTK_PIN_INFO(44, 0x110, 12, 1),
    MTK_PIN_INFO(45, 0x110, 13, 1),
    MTK_PIN_INFO(46, 0x110, 14, 1),
    MTK_PIN_INFO(47, 0x110, 15, 1),
    MTK_PIN_INFO(48, 0x110, 16, 1),
    MTK_PIN_INFO(49, 0x110, 17, 1),
    MTK_PIN_INFO(50, 0x110, 18, 1),
    MTK_PIN_INFO(51, 0x110, 19, 1),
    MTK_PIN_INFO(52, 0x110, 20, 1),
    MTK_PIN_INFO(53, 0x110, 21, 1),
    MTK_PIN_INFO(54, 0x110, 22, 1),
    MTK_PIN_INFO(55, 0x110, 23, 1),
    MTK_PIN_INFO(56, 0x110, 24, 1),
    MTK_PIN_INFO(57, 0x110, 25, 1),
    MTK_PIN_INFO(58, 0x110, 26, 1),
    MTK_PIN_INFO(59, 0x110, 27, 1),
    MTK_PIN_INFO(60, 0x110, 28, 1),
    MTK_PIN_INFO(61, 0x110, 29, 1),
    MTK_PIN_INFO(62, 0x110, 30, 1),
    MTK_PIN_INFO(63, 0x110, 31, 1),
    MTK_PIN_INFO(64, 0x120, 0, 1),
    MTK_PIN_INFO(65, 0x120, 1, 1),
    MTK_PIN_INFO(66, 0x120, 2, 1),
    MTK_PIN_INFO(67, 0x120, 3, 1),
    MTK_PIN_INFO(68, 0x120, 4, 1),
    MTK_PIN_INFO(69, 0x120, 5, 1),
    MTK_PIN_INFO(70, 0x120, 6, 1),
    MTK_PIN_INFO(71, 0x120, 7, 1),
    MTK_PIN_INFO(72, 0x120, 8, 1),
    MTK_PIN_INFO(73, 0x120, 9, 1),
    MTK_PIN_INFO(74, 0x120, 10, 1),
    MTK_PIN_INFO(75, 0x120, 11, 1),
    MTK_PIN_INFO(76, 0x120, 12, 1),
    MTK_PIN_INFO(77, 0x120, 13, 1),
    MTK_PIN_INFO(78, 0x120, 14, 1),
    MTK_PIN_INFO(79, 0x120, 15, 1),
    MTK_PIN_INFO(80, 0x120, 16, 1),
    MTK_PIN_INFO(81, 0x120, 17, 1),
    MTK_PIN_INFO(82, 0x120, 18, 1),
    MTK_PIN_INFO(83, 0x120, 19, 1),
    MTK_PIN_INFO(84, 0x120, 20, 1),
    MTK_PIN_INFO(85, 0x120, 21, 1),
    MTK_PIN_INFO(86, 0x120, 22, 1),
    MTK_PIN_INFO(87, 0x120, 23, 1),
    MTK_PIN_INFO(88, 0x120, 24, 1),
    MTK_PIN_INFO(89, 0x120, 25, 1),
    MTK_PIN_INFO(90, 0x120, 26, 1),
    MTK_PIN_INFO(91, 0x120, 27, 1),
    MTK_PIN_INFO(92, 0x120, 28, 1),
    MTK_PIN_INFO(93, 0x120, 29, 1),
    MTK_PIN_INFO(94, 0x120, 30, 1),
    MTK_PIN_INFO(95, 0x120, 31, 1),
    MTK_PIN_INFO(96, 0x130, 0, 1),
    MTK_PIN_INFO(97, 0x130, 1, 1),
    MTK_PIN_INFO(98, 0x130, 2, 1),
    MTK_PIN_INFO(99, 0x130, 3, 1),
    MTK_PIN_INFO(100, 0x130, 4, 1),
    MTK_PIN_INFO(101, 0x130, 5, 1),
    MTK_PIN_INFO(102, 0x130, 6, 1),
};

typedef struct _ubnt_gpio_pin {
    struct mtk_pin_info *dir;
    struct mtk_pin_info *mode;
    struct mtk_pin_info *in;
    struct mtk_pin_info *out;
} ubnt_gpio_pin;

static ubnt_gpio_pin i_led_gpio_white;
static ubnt_gpio_pin i_led_gpio_blue;
static ubnt_gpio_pin i_reset_button_gpio;

static ubnt_gpio_pin *led_gpio_white = &i_led_gpio_blue;
static ubnt_gpio_pin *led_gpio_blue = &i_led_gpio_white;
static ubnt_gpio_pin *reset_button_gpio = &i_reset_button_gpio;

int mtk_pinctrl_get_gpio_value(struct mtk_pin_info *pin)
{
    unsigned int reg_value, reg_get_addr;
    unsigned char bit_width, reg_bit;

    if (pin != NULL) {
        reg_get_addr = pin->offset;
        bit_width = pin->width;
        reg_bit = pin->bit;
        reg_value = GPIO_RD32(GPIO_BASE + reg_get_addr);
        return ((reg_value >> reg_bit) & ((1 << bit_width) - 1));
    } else {
        return -ERINVAL;
    }
}

int mtk_pinctrl_update_gpio_value(const struct mtk_pin_info *pin, int value)
{
    unsigned int reg_update_addr;
    unsigned int mask, reg_value;
    unsigned char bit_width;

    if (pin != NULL) {
        reg_update_addr = pin->offset;
        bit_width = pin->width;
        mask = ((1 << bit_width) - 1) << pin->bit;
        reg_value = GPIO_RD32(GPIO_BASE + reg_update_addr);
        reg_value &= ~(mask);
        reg_value |= (value << pin->bit);
        GPIO_WR32((GPIO_BASE + reg_update_addr),reg_value);
    } else {
        return -ERINVAL;
    }
    return 0;
}

int mtk_pinctrl_set_gpio_value(struct mtk_pin_info *pin, int value)
{
    unsigned int reg_set_addr, reg_rst_addr;

    if (pin != NULL) {
        reg_set_addr = pin->offset + 4;
        reg_rst_addr = pin->offset + 8;
        if (value)
           GPIO_SET_BITS((1L << pin->bit), (GPIO_BASE + reg_set_addr));
        else
           GPIO_SET_BITS((1L << pin->bit), (GPIO_BASE + reg_rst_addr));
    } else {
        return -ERINVAL;
    }
    return 0;
}

static int gpio_input_get(struct mtk_pin_info *pin)
{
    return mtk_pinctrl_get_gpio_value(pin);
}

static void gpio_output_set(struct mtk_pin_info *pin, int status)
{
    mtk_pinctrl_set_gpio_value(pin, status);
}

static void gpio_initialize(void) {
    const ubnt_bdinfo_t * bd;

    bd = board_identify(NULL);
    if (bd) {
        //printf("%s: vendorid=%04x, boardid=%04x, reset_button_gpio=%d\n", __FUNCTION__, bd->vendorid, bd->boardid, bd->reset_button_gpio);

        reset_button_gpio->mode = mtk_pinctrl_get_gpio_array(bd->reset_button_gpio,
                sizeof(mtk_pin_info_mode) / sizeof(struct mtk_pin_info),
                mtk_pin_info_mode
                );
        led_gpio_white->mode = mtk_pinctrl_get_gpio_array(bd->led_gpio_white,
                sizeof(mtk_pin_info_mode) / sizeof(struct mtk_pin_info),
                mtk_pin_info_mode
                );
        led_gpio_blue->mode = mtk_pinctrl_get_gpio_array(bd->led_gpio_blue,
                sizeof(mtk_pin_info_mode) / sizeof(struct mtk_pin_info),
                mtk_pin_info_mode
                );

        reset_button_gpio->dir = mtk_pinctrl_get_gpio_array(bd->reset_button_gpio,
                sizeof(mtk_pin_info_dir) / sizeof(struct mtk_pin_info),
                mtk_pin_info_dir
                );
        led_gpio_white->dir = mtk_pinctrl_get_gpio_array(bd->led_gpio_white,
                sizeof(mtk_pin_info_dir) / sizeof(struct mtk_pin_info),
                mtk_pin_info_dir
                );
        led_gpio_blue->dir = mtk_pinctrl_get_gpio_array(bd->led_gpio_blue,
                sizeof(mtk_pin_info_dir) / sizeof(struct mtk_pin_info),
                mtk_pin_info_dir
                );

        reset_button_gpio->in = mtk_pinctrl_get_gpio_array(bd->reset_button_gpio,
                sizeof(mtk_pin_info_datain) / sizeof(struct mtk_pin_info),
                mtk_pin_info_datain
                );
        led_gpio_white->out = mtk_pinctrl_get_gpio_array(bd->led_gpio_white,
                sizeof(mtk_pin_info_dataout) / sizeof(struct mtk_pin_info),
                mtk_pin_info_dataout
                );
        led_gpio_blue->out = mtk_pinctrl_get_gpio_array(bd->led_gpio_blue,
                sizeof(mtk_pin_info_dataout) / sizeof(struct mtk_pin_info),
                mtk_pin_info_dataout
                );

        // GPIO mode 
        mtk_pinctrl_update_gpio_value(led_gpio_blue->mode, 0x1);
        mtk_pinctrl_update_gpio_value(led_gpio_white->mode, 0x1);
        mtk_pinctrl_update_gpio_value(reset_button_gpio->mode, 0x1);

        mtk_pinctrl_update_gpio_value(led_gpio_blue->dir, 0x1);
        mtk_pinctrl_update_gpio_value(led_gpio_white->dir, 0x1);
        mtk_pinctrl_update_gpio_value(reset_button_gpio->dir, 0);


    } else {
        printf("board identity not found. skipping gpio_initialize\n");
        return;
    }
}


void unifi_set_led(int pattern)
{
    const ubnt_bdinfo_t* bd;

    bd = board_identify(NULL);
    if (bd->is_led_gpio_invert) {
        pattern = ~pattern;
    }
    gpio_output_set(led_gpio_blue->out, pattern & LED_BITMASK_BLUE);
    gpio_output_set(led_gpio_white->out, pattern & LED_BITMASK_WHITE);
}

static inline int poll_gpio_ms(struct mtk_pin_info *gpio, int duration, int freq, int revert) {
    int now;
    int cur_state, all_state;

    now = 0;
    all_state = revert ? (!gpio_input_get(gpio)) : gpio_input_get(gpio);
    while ((now < duration) && (all_state)) {
        cur_state = revert ? (!gpio_input_get(gpio)) : gpio_input_get(gpio);
        all_state &= cur_state;
        now += freq;
        udelay(freq * 1000);
    }

    return all_state;
}

ubnt_exec_t
ubnt_ureset_button(int argc, char *argv[]) {
    int reset_pressed;
    const ubnt_bdinfo_t* bd;
    ubnt_exec_t res;
    const char *env_rescue = getenv("do_urescue");
    const char *env_ipaddr = getenv("urescue_ip");
    char saved_ip[24];
    int rescue = env_rescue && !strcmp(env_rescue, "TRUE");

    if (env_ipaddr && strlen(env_ipaddr) < sizeof(saved_ip))
        strcpy(saved_ip, env_ipaddr);

    res.ret = 0;
    bd = board_identify(NULL);

    gpio_initialize();

    reset_pressed = poll_gpio_ms(reset_button_gpio->in, 3 * 1000, 50, 1);

    if (!reset_pressed && !rescue) {
#ifdef DEBUG
        printf ("keep cfg partition. \n");
#endif
        return res;
    }

    unifi_set_led(LED_STATE_ALL_OFF);

    //printf ("reset button pressed: clearing cfg partition!\n");
    unifi_set_led(LED_STATE_ALL_ON);
    //_do_clearcfg();
    setenv("ubnt_clearcfg", "TRUE");
    reset_pressed = poll_gpio_ms(reset_button_gpio->in, 3 * 1000, 50, 1);
    unifi_set_led(LED_STATE_ALL_OFF);

    if (!reset_pressed && !rescue) {
        return res;
    }

    reset_pressed = poll_gpio_ms(reset_button_gpio->in, 3 * 1000, 50, 1);

    if (!reset_pressed && !rescue) {
        return res;
    }

    //printf ("reset button pressed: clearing u-boot-env partition!\n");
    //_do_clear_ubootenv();
    if (!rescue)
        setenv("ubnt_clearenv", "TRUE");

    /* Restore the default env */
    //ubnt_env_init();
    set_do_urescue_flag();

    if (env_ipaddr)
        setenv("ipaddr", saved_ip);
    return res;
}

ubnt_exec_t
ubnt_usetled(int argc, char *argv[]) {
    int pattern;
    ubnt_exec_t res;

    res.ret = 0;
    if (argc <= 1) {
        res.ret = 1;
        return res;
    }

    pattern = simple_strtoul(argv[1], NULL, 10);
    unifi_set_led(pattern);
    return res;
}


ubnt_exec_t
ubnt_ubeaconpkt(int argc, char *argv[])
{
    int ver_len, payload_len;
    unsigned char *ver_p;
    unsigned char *ver_head_p;
    unsigned char *payload_p;
    unsigned char *pkt;
    unsigned short int sbuf;
    unsigned char *c;
    int intbuf;
    const ubnt_bdinfo_t* bd;
    unsigned long bip;
    int *out_len;
    ubnt_exec_t res;

    /* Init the return structure */
    res.beacon_pkt.ret = 0;
    res.beacon_pkt.len = 0;
    res.beacon_pkt.ptr = NULL;

    out_len = &res.beacon_pkt.len;

    if (argc < 3) {
        res.beacon_pkt.ret = -1;
        return res;
    }

    bip = simple_strtoul(argv[1], NULL, 16);

    /* U_BOOT_VERSION */
    ver_len = strlen(argv[2]);
    ver_head_p = (unsigned char *)argv[2];
    ver_p = (unsigned char *)strchr((char *)ver_head_p, ' ');
    ver_p++;
    ver_len -= (ver_p - ver_head_p);

    bd = board_identify(NULL);

    if (bd->vendorid != BDINFO_VENDORID_UBNT) {
        res.beacon_pkt.ret = -1;
        return res;
    }

    payload_len = 4 + 13 + 9 + 5 + 7 +(3 + ver_len);

    payload_p = malloc(payload_len);
    if (!payload_p) {
        res.beacon_pkt.ret = -2;
        return res;
    }

    c = (unsigned char *)&sbuf;

    pkt = payload_p;

    *pkt++ = 0x02;      /* version */
    *pkt++ = 0x07;      /* urescue beacon */
    sbuf = htons(payload_len - 4);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    *pkt++ = 0x02;      /* addresses */
    sbuf = htons(10);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    /* MAC ddress */
    *pkt++ = bd->eth0.hwaddr[0];
    *pkt++ = bd->eth0.hwaddr[1];
    *pkt++ = bd->eth0.hwaddr[2];
    *pkt++ = bd->eth0.hwaddr[3];
    *pkt++ = bd->eth0.hwaddr[4];
    *pkt++ = bd->eth0.hwaddr[5];

    /* IP address */
    c = (unsigned char *)&bip;
    *pkt++ = *c;
    *pkt++ = *(c+1);
    *pkt++ = *(c+2);
    *pkt++ = *(c+3);
    c = (unsigned char *)&sbuf;

    *pkt++ = 0x01;      /* Device ID */
    sbuf = htons(6);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    /* Device ID */
    *pkt++ = bd->eth0.hwaddr[0];
    *pkt++ = bd->eth0.hwaddr[1];
    *pkt++ = bd->eth0.hwaddr[2];
    *pkt++ = bd->eth0.hwaddr[3];
    *pkt++ = bd->eth0.hwaddr[4];
    *pkt++ = bd->eth0.hwaddr[5];

    *pkt++ = 0x10;      /* System ID */
    sbuf = htons(2);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    /* System ID */
    sbuf = htons(bd->boardid);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    *pkt++ = 0x11;      /* BOM Revision */
    sbuf = htons(4);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    /* System ID */
    intbuf = htonl(bd->bomrev);
    c = (unsigned char *)&intbuf;
    *pkt++ = *c;
    *pkt++ = *(c+1);
    *pkt++ = *(c+2);
    *pkt++ = *(c+3);
    c = (unsigned char *)&sbuf;

    *pkt++ = 0x03;      /* Bootloader Version */
    sbuf = htons(ver_len);
    *pkt++ = *c;
    *pkt++ = *(c+1);

    memcpy(pkt, ver_p, ver_len);

    *out_len = payload_len;

    res.beacon_pkt.ptr = payload_p;
    return res;
}

static const char*
wifi_type_to_name(int type) {
    switch (type) {
    case WIFI_TYPE_UNKNOWN:
    default:
        return "unknown";
    }
    return NULL;
}


void
ucheckboard(void) {
    const ubnt_bdinfo_t* b;
    char buf[50];

    buf[0] = 0x00;
    b = board_identify(NULL);
    // jeffrey. In this moment, gpio is not initialized
    //unifi_set_led(0);
    printf("Board: Ubiquiti Networks %s board (",
        cpu_type_to_name(b->cpu_type));
    board_dump_ids(buf, b);
    printf(")\n");
}


ubnt_exec_t
ubnt_ucheckboard(int argc, char *argv[])
{
    ubnt_exec_t res;
    res.ret = 0;

    ucheckboard();

    return res;
}

int
ubnt_flash(char *mtd_name, unsigned int *data, int size) {
#if 0
    flash_uinfo_t* flinfo = &flash_info[0];
    const mtd_parts_t *mtd = (const mtd_parts_t*) find_mtd_by_name(mtd_name);
    int rc=0;

    if (!mtd) {
        printf("Could find the partition name = %s \n", mtd_name);
        return -1;
    }

    if (size > mtd->size) {
        printf("Data size exceeds the parition size \n");
        return -1;
    }

    flash_erase(flinfo, mtd->start_sector, mtd->end_sector);
    rc = write_buff(flinfo, (unsigned char *)data, (unsigned char *)mtd->start_address, size);

    return rc;
#endif
    return 0;
}

int
ubnt_dual_flash(char *mtd_name, unsigned int *data, int size) {
#if 0
    flash_uinfo_t* flinfo = &flash_info[0];
    const mtd_parts_t *mtd = (const mtd_parts_t*) find_mtd_by_name(mtd_name);
    int rc=0;

    if (!mtd) {
        printf("Could find the partition name = %s \n", mtd_name);
        return -1;
    }

    if (size > mtd->size) {
        printf("Data size exceeds the parition size \n");
        return -1;
    }

    /* Only one image of kernel is passed. So apply this one image
    ** for both partitions
    */
    if (!strncmp(mtd_name, "kernel", 6)) {
        if ((rc=ubnt_flash_update("kernel0", data, size))) {
            return rc;
        }

        if ((rc=ubnt_flash_update("kernel1", data, size))) {
            return rc;
        }
    }
    else if (!strncmp(mtd_name, "u-boot", 6)) {
        if ((rc=ubnt_flash_update("u-boot", data, size))) {
            return rc;
        }
    }

    return rc;

#endif
    return 0;
}

#define ENV_KERNEL0         "kernel0"
#define ENV_KERNEL1         "kernel1"
#define ENV_FORCE_BOOTSEL   "force_bootsel"
#define ENV_BOOTARG         "bootargs"
#define ENV_FLASH_BOOT_PART "flash_boot_addr"


//#define UBNT_PART_OFF(x) (get_mtd_saddress((0==x?"kernel0":"kernel1")))
#define UBNT_PART_NAME(x) (0 == x? ENV_KERNEL0:ENV_KERNEL1)
#define UBNT_PART_BOOTID(x) (0 == x? "0":"1")

static unsigned int
load_kernel_to_ram(unsigned int *flash_img_ptr)
{
    const mtd_parts_t* mtd;
    unsigned long addr = 0;
    struct spi_flash *flash;
    char *addr_str;
    char str[16];
    int ret, size=0;

    addr_str = getenv("loadaddr");

    if (addr_str != NULL) {
        addr = strtoul (addr_str, NULL, 16);
    } else {
        addr = CONFIG_SYS_LOAD_ADDR;
    }

    mtd = (const mtd_parts_t*) find_mtd_by_name(ENV_KERNEL0);
    size = mtd->size;

    printf("Loading Kernel Image @ %x, size = %d \n", addr, size);

    memcpy((unsigned int *)addr, (void *) flash_img_ptr, size);

    return addr;
}

static int
uimage_get_data(const image_header_t *hdr) {
    return (unsigned int) hdr + sizeof(image_header_t);
}

static int
uimage_get_data_size(const image_header_t *hdr) {
    return ntohl(hdr->ih_size);
}

static int uimage_get_dcrc(image_header_t *hdr) {
    return ntohl(hdr->ih_dcrc);
}

static int
uimg_check_data_crc(const image_header_t *hdr)
{
    unsigned char *data = (unsigned char *)uimage_get_data(hdr);
    unsigned long crc;

    crc = ucrc32(data, uimage_get_data_size(hdr));

    if (crc != uimage_get_dcrc(hdr)) {
        return 0;
    }
    return 1;
}

static int uimage_check_magic(const image_header_t *hdr) {
    return (ntohl(hdr->ih_magic) == IH_MAGIC);
}

static int uimage_set_hcrc(image_header_t *hdr, unsigned int v) {
    hdr->ih_hcrc = htonl(v);
    return 0;
}

static int
img_check_hdr_crc(const image_header_t *hdr) {
    image_header_t *tmp, tmp_hdr;
    unsigned int crc;

    tmp = &tmp_hdr;

    memcpy(tmp, hdr, sizeof(image_header_t));
    uimage_set_hcrc(tmp, 0);

    crc = ucrc32(tmp, sizeof(image_header_t));

    if (crc != ntohl(hdr->ih_hcrc)) {
        return 0;
    }
    return 1;
}


static unsigned long
kernel_verify(int bootsel) {
#if 0
    unsigned int *flash_addr = (unsigned int *)UBNT_PART_OFF(bootsel);
    image_header_t *hdr = (image_header_t *)load_kernel_to_ram(flash_addr);

    if (!hdr) return NULL;

    printf("Verifying '%s' parition:", UBNT_PART_NAME(bootsel));

    if (!uimage_check_magic(hdr)) {
        printf("Bad Magic Number\n");
        return NULL;
    }

    if (!img_check_hdr_crc(hdr)) {
        printf("Bad header CRC \n");
        return NULL;
    }

    if (!img_check_data_crc(hdr)) {
        printf("Bad Data CRC \n");
        return NULL;
    }
    printf("OK\n");

    return (unsigned long) hdr;
#endif
    return NULL;
}


int ubnt_ubntboot(int argc, char * const argv[]) {
    int ret=0;
    char *bootargs_str;
    char *addr_str, *force_bootsel, str[64], bootargs[254];
    unsigned long addr, boot_sel[24];
    unsigned int bootsel=0, i=0;

#define STR(x) #x
#define STRING(x) STR(x)

#if 0
    /* For 8MB flash with single boot paritition */
    if (!is_dual_boot()) {
        setenv(ENV_FLASH_BOOT_PART, STRING(KERNEL_FLASH_ADDR_8MB));
    } else {
        bootargs_str = getenv(ENV_BOOTARG);

        force_bootsel = getenv(ENV_FORCE_BOOTSEL);

        if (!force_bootsel)  {
            /* Get boot partition from bootsel flash block */
            ubnt_get_bootsel(&bootsel);
        } else {
            bootsel = *force_bootsel == '1'? 1 : 0;
            printf("Note: Bootselection forced to %d \n", bootsel);
        }

        if ((addr = kernel_verify(bootsel)) != NULL) {
            strcpy(bootargs, bootargs_str);
            strcat(bootargs, " ubntbootid=");
            strcat(bootargs, UBNT_PART_BOOTID(bootsel));
            setenv(ENV_FLASH_BOOT_PART,
               bootsel?STRING(KERNEL1_FLASH_ADDR_16MB):STRING(KERNEL0_FLASH_ADDR_16MB));
            setenv(ENV_BOOTARG, bootargs);
        } else if ((addr = kernel_verify(!bootsel)) != NULL) {
            strcpy(bootargs, bootargs_str);
            strcat(bootargs, " ubntbootid=");
            strcat(bootargs, UBNT_PART_BOOTID(!bootsel));
            setenv(ENV_FLASH_BOOT_PART,
              !bootsel?STRING(KERNEL1_FLASH_ADDR_16MB):STRING(KERNEL0_FLASH_ADDR_16MB));
            setenv(ENV_BOOTARG, bootargs);
        } else {
            /* TODO: go into urescue mode */
            printf(" FATAL: No parition is good. \n");
            return -1;
        }

    }
#endif
    return ret;
}

ubnt_exec_t
ubnt_uwrite(int argc, char *argv[]) {
    ubnt_exec_t res;
    int rc=0, bl=1;
    unsigned  long *data, size;
    char *fa_env, *fs_env;

    res.ret = 0;

#if 0
    if (!(strcmp(argv[1], "-f"))) {
        bl = 0;
    }

    flash_status_display(1);

    fa_env = getenv("fileaddr");
    fs_env = getenv("filesize");

    /* Urescue did not run */
    if (!fa_env || !fs_env) {
#ifdef DEBUG
        printf("%s: Nothing to flash, exiting \n", __func__);
#endif
        return res;
    }

    data = (unsigned long *)strtoul(fa_env, NULL, 16);

    size = (unsigned long *)strtoul(fs_env, NULL, 16);

    rc = ubnt_fw_check(data, size);

    if (!rc) {
        rc = ubnt_update_fw(data, size, bl);
    }

    if (rc) {
        res.ret = -1;
    }

    if (!rc)
        ath_reset();

ubnt_err:
    flash_status_display(0);
#endif
    return res;
}

static void detect_reset_button(void) {
    char * ubnt_flag;

    ubnt_ureset_button(0, NULL);
    ubnt_flag = getenv("do_urescue");
    if (ubnt_flag && (!strncmp(ubnt_flag, "TRUE", 4))) {
        set_ubnt_app_state(UAPP_STATE_DO_URESCUE);
    } else {
        set_ubnt_app_state(UAPP_STATE_READ_KERNEL_FIRST);
    }
}

static int is_legacy_image(image_header_t *hdr) {
    char *magic = (char *)(&hdr->ih_magic);
    int i;

    for (i = 0; i < sizeof(legacy_magic_number); i++) {
        if (magic[i] != legacy_magic_number[i]) {
            return 0;
        }
    }
    return 1;
}

static int verify_kernel_image(void) {
    image_header_t *hdr = (image_header_t *)CONFIG_SYS_LOAD_ADDR;

    if ( is_legacy_image(hdr) ) {
        // Legacy
        if (!img_check_hdr_crc(hdr)) {
            printf("Bad header CRC.\n");
            return -1;
        }

        if (!uimg_check_data_crc(hdr)) {
            printf("Bad Data CRC.\n");
            return -1;
        }
        return 0;
    }

    printf("Bad Magic Number.\n");

    return -1;
}

static int process_urescue_file(void) {
    int ret = 0;
    char * env_var_str;
    int fileaddr;
    int filesize;
    fw_t * fw_info;
    int i;

    env_var_str = getenv("fileaddr");
    if ( !env_var_str ) {
        printf("No download file address.\n");
        return -1;
    }
    fileaddr = simple_strtoul(env_var_str, NULL, 16);

    env_var_str = getenv("filesize");
    if ( !env_var_str ) {
        printf("No download file size.\n");
        return -1;
    }
    filesize = simple_strtoul(env_var_str, NULL, 16);

    ret = ubnt_fw_check((void*)fileaddr, filesize);

    if (!ret) {
        fw_info = ubnt_fw_get_info();
        if ( (fw_info->part_count < 1) || (fw_info->parts[0].valid == 0) ) {
            printf("Invalid firmware file.\n");
            return -1;
        }

        p_share_data->kernel_index = -1;
        for(i = 0; i < fw_info->part_count; i++) {
            if ( 0
                || (!strncmp(fw_info->parts[i].header->name, UBNT_PART_KERNEL0_NAME, strlen(UBNT_PART_KERNEL0_NAME)))
                ) {
                p_share_data->update.kernel0.addr = (unsigned int)fw_info->parts[i].data;
                p_share_data->update.kernel0.size = ntohl(fw_info->parts[i].header->length);
                p_share_data->kernel_index = 0;
            } else if (!strncmp(fw_info->parts[i].header->name, UBNT_PART_KERNEL1_NAME, strlen(UBNT_PART_KERNEL1_NAME))) {
                p_share_data->update.kernel1.addr = (unsigned int)fw_info->parts[i].data;
                p_share_data->update.kernel1.size = ntohl(fw_info->parts[i].header->length);
                if( p_share_data->kernel_index < 0 ) {
                    /* use kernel0 if both kernel0 and kernel1 are available */
                    p_share_data->kernel_index = 1;
                }
            } else if (!strncmp(fw_info->parts[i].header->name, UBNT_PART_UBOOT_NAME, strlen(UBNT_PART_UBOOT_NAME))) {
                const char *net_urescue = getenv("do_net_urescue");
                const char *net_urescue_bl = getenv("net_urescue_bl");

                int update_uboot = 0;

                if ((p_internal_data->flags & UAPP_FLAG_ALLOW_UPDATE_CRITICAL_PARTITION) != 0)
                    update_uboot = 1;
                if (net_urescue && strncmp(net_urescue, "TRUE", 4) == 0 &&
                        (!net_urescue_bl || strncmp(net_urescue_bl, "TRUE", 4) != 0))
                    update_uboot = 0;

                if (update_uboot) {
                    p_share_data->update.uboot.addr = (unsigned int)fw_info->parts[i].data;
                    p_share_data->update.uboot.size = ntohl(fw_info->parts[i].header->length);
                } else {
                    printf("updating uboot not allowed, skip.\n");
                }
            }
        }

#ifdef DEBUG
        printf("f/w address: 0x%08x, size: 0x%8x\n", p_share_data->update.kernel0.addr, p_share_data->update.kernel0.size);
#endif
        if( p_share_data->kernel_index >= 0) {
            /* need to update bootselect */
            if( sizeof(ubnt_bs_t) > sizeof(ubnt_persistent_data.util) ) {
                printf("*Warning*: No room to store bootselect info.\n");
            } else {
                ubnt_bs_t *p_bootselect;
                p_bootselect = (ubnt_bs_t *)(&ubnt_persistent_data.util[0]);
                memset(p_bootselect, 0, sizeof(*p_bootselect));
                p_bootselect->magic = UBNT_BS_MAGIC;
                p_bootselect->bootsel = p_share_data->kernel_index;
                p_share_data->update.bootselect.addr = (unsigned int) p_bootselect;
                p_share_data->update.bootselect.size = sizeof(ubnt_bs_t);
            }
        } else {
            printf("No kernel image detected.\n");
            ret = -1;
        }
    }
    return ret;
}

#define UBNTBOOTID_ARG_STR " ubntbootid="
static void set_ubntbootid(void) {
    char bootargs[254];
    char * env_var_str;
    unsigned int bootargs_len;

    env_var_str = getenv(ENV_BOOTARG);
    bootargs_len = strlen(env_var_str) + strlen(UBNTBOOTID_ARG_STR) + 2;
    if( bootargs_len > sizeof(bootargs) ) {
        printf("**WARNING**, Buffer too small for bootargs.\n");
        return;
    }

    strcpy(bootargs, env_var_str);
    strcat(bootargs, UBNTBOOTID_ARG_STR);

    if( 0 == p_share_data->kernel_index ) {
        strcat(bootargs, "0");
    } else {
        strcat(bootargs, "1");
    }
    setenv(ENV_BOOTARG, bootargs);
}

#define UBNTBOOTCONFIG_ARG_STR "bootconfig"
static void set_ubntsystemid(void) {
    const ubnt_bdinfo_t *bd = board_identify(NULL);
    const char *config;

    if (bd->vendorid != BDINFO_VENDORID_UBNT) {
        config = "-1";
        setenv(UBNTBOOTCONFIG_ARG_STR, config);
        return;
    }

    switch(bd->boardid) {
        case SYSTEMID_UAP6PRO:
            config = "1";
            break;
        default:
            config = "-1";
    }

    setenv(UBNTBOOTCONFIG_ARG_STR, config);
}


static void urescue_continue(int ubnt_event) {
    int ret = 0;
    ubnt_led_pattern_t * p_led_pattern;

    p_led_pattern = (ubnt_led_pattern_t *)(p_internal_data->led_pattern);

#ifdef DEBUG
    printf("urescue_continue in, event: %d\n", ubnt_event);
#endif

    switch (ubnt_event ) {
    case UBNT_EVENT_TFTP_TIMEOUT:
        if( p_led_pattern != &led_pattern_tftp_waiting ) {
            p_internal_data->led_pattern = (unsigned int)(&led_pattern_tftp_waiting);
            p_internal_data->led_current_step = 0;
        }
        update_led_state();
        break;
    case UBNT_EVENT_TFTP_RECEIVING:
        if( p_led_pattern != &led_pattern_tftp_receiving ) {
            p_internal_data->led_pattern = (unsigned int)(&led_pattern_tftp_receiving);
            p_internal_data->led_current_step = 0;
        }
        update_led_state();
        break;
    case UBNT_EVENT_TFTP_DONE:
        unifi_set_led( LED_STATE_ALL_OFF );
        ret = process_urescue_file();
        if(ret == 0) {
            /* kernel is good */
            setenv("ubnt_do_boot", "TRUE");
            set_ubntbootid();
            set_ubnt_app_state(UAPP_STATE_UPDATE_FIRMWARE);
        }
        break;
    default:
        printf("**WARNING**, urescue_continue, un-handled event: %d.\n", ubnt_event);
        break;
    }
}

static void read_kernel_continue(int is_first, int ubnt_event) {
    int kernel_invalid = 0;
    ubnt_led_pattern_t * p_led_pattern;

    switch (ubnt_event) {
    case UBNT_EVENT_READING_FLASH:
        /* for LED indication only */
        p_led_pattern = (ubnt_led_pattern_t *)(p_internal_data->led_pattern);
        if( p_led_pattern != &led_pattern_firmware_reading ) {
            p_internal_data->led_pattern = (unsigned int)(&led_pattern_firmware_reading);
            p_internal_data->led_current_step = 0;
        }
        update_led_state();
        return;
        break;
    case UBNT_EVENT_KERNEL_VALID:
        kernel_invalid = 0;
        break;
    case UBNT_EVENT_KERNEL_INVALID:
        kernel_invalid = 1;
        break;
    default:
        kernel_invalid = verify_kernel_image();
        break;
    }

    if (kernel_invalid == 0) {
        /* kernel is good */
        setenv("ubnt_do_boot", "TRUE");
        set_ubntbootid();
        set_ubntsystemid();
        set_ubnt_app_state(UAPP_STATE_BOOT);
        unifi_set_led( LED_STATE_ALL_OFF );
    } else {
        /* kernel not valid */
        if(is_first) {
            /* read next kernel */
            if( p_share_data->kernel_index == 0 ) {
                p_share_data->kernel_index = 1;
            } else {
                p_share_data->kernel_index = 0;
            }
            set_ubnt_app_state(UAPP_STATE_READ_KERNEL_SECOND);
        } else {
            /* both kernel not valid */
            set_do_urescue_flag();
            set_ubnt_app_state(UAPP_STATE_DO_URESCUE);
        }
    }
}

static void update_firmware_continue(int ubnt_event) {
    ubnt_led_pattern_t * p_led_pattern;

    p_led_pattern = (ubnt_led_pattern_t *)(p_internal_data->led_pattern);
    if( p_led_pattern != &led_pattern_firmware_updating ) {
        p_internal_data->led_pattern = (unsigned int)(&led_pattern_firmware_updating);
        p_internal_data->led_current_step = 0;
    }
    update_led_state();
}

ubnt_exec_t
ubnt_uappcontinue(int argc, char *argv[]) {
    int state;
    int is_first;
    int ubnt_event = UBNT_EVENT_UNKNOWN;
    ubnt_exec_t res;

    if (argc >= 2) {
        ubnt_event = simple_strtoul(argv[1], NULL, 0);
    }

    state = get_ubnt_app_state();
    switch (state ) {
    case UAPP_STATE_INIT_DONE:
        detect_reset_button();
        break;
    case UAPP_STATE_DO_URESCUE:
        urescue_continue(ubnt_event);
        break;
    case UAPP_STATE_READ_KERNEL_FIRST:
        is_first = 1;
        read_kernel_continue(is_first, ubnt_event);
        break;
    case UAPP_STATE_READ_KERNEL_SECOND:
        is_first = 0;
        read_kernel_continue(is_first, ubnt_event);
        break;
    case UAPP_STATE_UPDATE_FIRMWARE:
        update_firmware_continue(ubnt_event);
        break;
    default:
        break;
    }
    res.ret = 0;
    return res;
}

static int do_uramload(unsigned int fileaddr, unsigned int filesize)
{
    fw_t * fw_info;
    int i;

    if (!ubnt_fw_check((void*)fileaddr, filesize)) {
        fw_info = ubnt_fw_get_info();
        if ( (fw_info->part_count < 1) || (fw_info->parts[0].valid == 0) ) {
            printf("Attempted to ramload an invalid firmware file.\n");
            return -1;
        }

        for(i = 0; i < fw_info->part_count; i++) {
            if (!strncmp(fw_info->parts[i].header->name, UBNT_PART_KERNEL0_NAME, strlen(UBNT_PART_KERNEL0_NAME))) {
                p_share_data->kernel_addr = (unsigned int)fw_info->parts[i].data;
                set_ubntbootid();
                set_ubntsystemid();
                printf("Found ramload "UBNT_PART_KERNEL0_NAME" address: 0x%08x, size: 0x%8x\n",
                        p_share_data->kernel_addr, fw_info->parts[i].header->length);
                return 0;
            }
        }
        printf("No "UBNT_PART_KERNEL0_NAME" found in ramload image\n");
    }

    return -1;
}

ubnt_exec_t
ubnt_uramload(int argc, char *argv[]) {
    ubnt_exec_t res;
    unsigned int fileaddr;
    unsigned int filesize;
    if (argc != 3) {
        printf("uramload requires 2 parameters\n");
        res.ret = -1;
        return res;
    }
    fileaddr = simple_strtoul(argv[1], NULL, 0);
    filesize = simple_strtoul(argv[2], NULL, 0);
    res.ret = do_uramload(fileaddr, filesize);
    return res;
}
