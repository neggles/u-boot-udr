#include <ubnt_config.h>
#include <types.h>
#include <ath_flash.h>
#include <ubnt_flash_protect.h>
#include <ubnt_flash.h>

#ifdef CONFIG_FLASH_WP

#if defined( __KERNEL__) && defined(LINUX_VERSION_CODE)
#define fp_printf printk
#else
#define fp_printf printf
#endif

#ifdef FP_DEBUG
#define fp_debug_printf fp_printf
#else
#define fp_debug_printf(...)
#endif // #ifdef DEBUG

#define RDSR_SRWD_SHIFT 7
#define RDSR_BP2_SHIFT  4
#define RDSR_BP1_SHIFT  3
#define RDSR_BP0_SHIFT  2

#define SR_SRWP (1 << RDSR_SRWD_SHIFT)
#define SR_BP_BITS \
      ((1 << RDSR_BP2_SHIFT) \
      | (1 << RDSR_BP1_SHIFT) \
      | (1 << RDSR_BP0_SHIFT))

#define SR_SRWP_AND_BP_BITS   (SR_SRWP|SR_BP_BITS)

#ifndef ART_BUILD
    #define ALWAYS_HPM_LOCK 1
#else // #ifndef ART_BUILD
    #define ALWAYS_HPM_LOCK 0
#endif // #ifndef ART_BUILD

#ifndef FLASH_WP_GPIO
#error Please set FLASH_WP_GPIO with CONFIG_FLASH_WP enabled.
#endif // #ifndef FLASH_WP_GPIO

uint8_t ath_spi_read_status(void) {
	u32 rd;
	ath_reg_rmw_set(ATH_SPI_FS, 1);
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
    ath_spi_bit_banger(ATH_SPI_CMD_RD_STATUS);
    ath_spi_delay_8();
    ath_spi_go();
    ath_spi_poll();
    rd = ath_reg_rd(ATH_SPI_RD_STATUS);
    ath_spi_done();
    return rd & 0xFF;
}

void ath_spi_write_status(uint8_t value) {
    ath_spi_write_enable();
    ath_spi_bit_banger(ATH_SPI_CMD_WRITE_SR);
    ath_spi_bit_banger(value);
    ath_spi_go();
    ath_spi_poll();
    ath_spi_done();
}


#if FLASH_WP_GPIO == 0
    #define FLASH_WP_FUNCTION_REGISTER ATH_GPIO_OUT_FUNCTION0
    #define FLASH_WP_FUNCTION_SHIFT    0
#elif FLASH_WP_GPIO == 16
    #define FLASH_WP_FUNCTION_REGISTER ATH_GPIO_OUT_FUNCTION4
    #define FLASH_WP_FUNCTION_SHIFT    0
#else
    #error ERROR: Only GPIO 0 and 16 are supported for FLASH_WP_GPIO.  Add more as necessary.
#endif


/* Is the WP signal ON (status register write protected if SRWP bit is enabled) */
void ubnt_flash_hw_write_protect(int protect) {
    ath_reg_rmw_clear(FLASH_WP_FUNCTION_REGISTER, (0xFF << FLASH_WP_FUNCTION_SHIFT));
    ath_reg_rmw_clear(ATH_GPIO_OE, (1 << FLASH_WP_GPIO));
    ath_reg_rmw_clear(ATH_GPIO_INT_ENABLE, (1 << FLASH_WP_GPIO));
    ath_reg_wr(protect ? ATH_GPIO_SET : ATH_GPIO_CLEAR,   (1<<FLASH_WP_GPIO)); // LOW=unlocked HIGH=locked
}

/* Is the WP signal OFF (status register not protected) */
int ubnt_get_hw_write_protect(void) {
    return !!(ath_reg_rd(ATH_GPIO_OUT) & (1<<FLASH_WP_GPIO));
}

/* Is the WP signal enabled */
int ubnt_get_srwp(void) {
    return !!(ath_spi_read_status() & SR_SRWP);
}

/* Are one or more blocks write protected? */
int ubnt_get_bp(void) {
    return !!(ath_spi_read_status() & SR_BP_BITS);
}

/*

In production mode (ALWAYS_HPM_LOCK == 1),
we lower the hardware WP signal only long enough to set/clear
all block protect bits and we will keep the hardware WP feature
bit set (SRWP=1) and raise the HW WP signal after every change.

In ART mode (ALWAYS_HPM_LOCK == 0), we will always disable the
hardware WP signal feature bit (SRWP=0) and lower the HW WP
signal.

This function will:

    1.  Drop HW WP signal (if hardware WP signal is supported (hw_wp != 0))

    2.  Set HW WP signal feature enable bit (SRWP = 1),
        unless we are disabling protection in ART mode (ALWAYS_HPM_LOCK == 0).

    3.  Set/clear all block protection bits (for enabled/disabled respectively)

    4.  Raise WP signal (if hardware WP signal is supported (hw_wp != 0)),
        unless we are disabling protection in ART mode (ALWAYS_HPM_LOCK == 0).

*/
void ubnt_set_flash_protection(int hw_wp, int enabled) {
    uint8_t hw_lock = (hw_wp && (enabled || ALWAYS_HPM_LOCK));
    uint8_t new_sr  =
        (hw_lock ? SR_SRWP    : 0)
    |   (enabled ? SR_BP_BITS : 0)
    ;

    fp_debug_printf(
        "Flash protection %s [%s%s%s].\n"
        , enabled ? "ENABLED" : "DISABLED"
        , hw_lock ? "HPM" : "SPM"
        , hw_wp ? (hw_lock ? "+Locked" : "+Unlocked") : ""
        , (hw_wp && !enabled)
            ? (hw_lock
                ? ", HPM on always"
                : ", HPM on if protected"
               )
            : ""
        );
    /* Drop WP always - will setup the GPIO as necessary, the first time */
    if (hw_wp) {
        ubnt_flash_hw_write_protect(0);
    }
    /* Set the status register if necessary */
    if (ath_spi_read_status() != new_sr) {
        ath_spi_write_status(new_sr);
    }
    /* Raise WP (as necessary) */
    if (hw_lock) {
        ubnt_flash_hw_write_protect(1);
    }
}

void ubnt_flash_enable_protection(int hw_wp) {
    ubnt_set_flash_protection(hw_wp,1);
}

void ubnt_flash_disable_protection(int hw_wp) {
    ubnt_set_flash_protection(hw_wp,0);
}

#define bool_t int
void ubnt_flash_print_protection_status(void) {
    uint8_t r                   = ath_spi_read_status();
    bool_t   srwd                = ubnt_get_srwp();
    bool_t   bp2                 = !!(r & (1 << RDSR_BP2_SHIFT));
    bool_t   bp1                 = !!(r & (1 << RDSR_BP1_SHIFT));
    bool_t   bp0                 = !!(r & (1 << RDSR_BP0_SHIFT));
    // NB: Not spending time making this dynamic - hard coded for GPIO16
    int      hw_wp               = 1;//srwd; // TODO: Board detection should determine this on production u-boot
    bool_t   wp_signal_enabled   = !(ath_reg_rd(ATH_GPIO_OE) & (1 << FLASH_WP_GPIO));
    bool_t   wp_signal_setting   = 0 != (ath_reg_rd(ATH_GPIO_OUT) & (1 << FLASH_WP_GPIO));
    uint8_t wp_signal_func      = (ath_reg_rd(FLASH_WP_FUNCTION_REGISTER) & (0xFF << FLASH_WP_FUNCTION_SHIFT)) >> FLASH_WP_FUNCTION_SHIFT;
    const char* protection_mode_desc  = "Software Protection Mode [SPM]";
    bool_t      block_protection = (bp0 || bp1 || bp2);
    bool_t      block_protection_partial  = block_protection && !(bp0 && bp1 && bp2);
    bool_t      hw_lock          = wp_signal_enabled && (wp_signal_setting != 0) && (wp_signal_func == 0);
    const char *hw_locked_desc   = "";
    const char *block_protection_desc = "OFF";

    if(block_protection_partial) {
        block_protection_desc = "PARTIALLY ON";
    } else if (block_protection) {
        block_protection_desc = "ON";
    } else {
        block_protection_desc = "OFF";
    }

    // If we have hardware lock, show it
    if (hw_lock && srwd) {
        protection_mode_desc = "Hardware Protection Mode [HPM]";
        hw_locked_desc = "Locked";
    } else if(hw_lock) {
        hw_locked_desc = "Unlocked [SRWD not set]";
    } else if(srwd) {
        hw_locked_desc = "Unlocked [HW WP off]";
    } else {
        hw_locked_desc = "Unlocked [SRWD not set, HW WP off]";
    }

    fp_printf("-------------------------------------------------------\n");
    fp_printf("Flash Protection Status\n");
    fp_printf("-------------------------------------------------------\n");
    fp_printf("\n");
    fp_printf("Flash protection.............%s [%s%s%s].\n"
        , block_protection ? (block_protection_partial ? "Partially-Writable" : "Read-Only") : "Writable"
        , hw_lock ? "HPM" : "SPM"
        , hw_wp ? (hw_lock ? "+Locked" : "+Unlocked") : ""
        , (hw_wp && !block_protection)
            ? (hw_lock
                ? ", HPM on always"
                : ", HPM on if protected"
               )
            : ""
        );
    fp_printf("Block Protection.............%s\n", block_protection_desc);
    fp_printf("Mode.........................%s\n", protection_mode_desc);
    fp_printf("HW Locked....................%s\n", hw_locked_desc);
    fp_printf("\n");
    fp_debug_printf("Diagnostic Details: \n");
    fp_debug_printf("\n");
    fp_debug_printf("SRWD.........................%d\n", srwd);
    fp_debug_printf("RDSR.........................0x%02x\n", r);
    fp_debug_printf("Block Protection.............BP2=%d BP1=%d BP0=%d%s\n", bp2, bp1, bp0, (block_protection_partial ? " <<ERROR>>" : ""));
    fp_debug_printf("HW WP GPIO...................%d\n", FLASH_WP_GPIO);
    fp_debug_printf("HW WP GPIO Output Mode.......%s\n", wp_signal_enabled ? "Yes" : "No <<ERROR>>");
    fp_debug_printf("HW WP GPIO Setting...........%s\n", wp_signal_setting ? "Enabled" : "Disabled");
    fp_debug_printf("HW WP GPIO Function..........0x%02x%s\n", wp_signal_func, (wp_signal_func ? " <<ERROR>>" : ""));
    fp_debug_printf("HW ATH GPIO OE...............0x%08x\n", ath_reg_rd(ATH_GPIO_OE));
    fp_debug_printf("HW ATH GPIO OUT..............0x%08x\n", ath_reg_rd(ATH_GPIO_OUT));
    fp_debug_printf("-------------------------------------------------------\n");
    fp_debug_printf("\n");
}

#ifdef ART_BUILD
void ubnt_flash_factory_reset_for_art(void) {
    fp_printf("ART Build - Factory reset flash protection.\n");
    /*
    The status register write protect bit will be set in the
    flash chip by **production** u-boot/kernel when WP hardware
    is present.

    We use this to determine when to use flash WP support, since
    calibration data or board data is not always present in ART.

    GPIO0 is connected to either WP or PHY external reset (AR8035)
    so we are safe to always configure as an output and set it
    HIGH at this point in initialization.

    This will either harmlessly set external Ethernet reset
    signal high (out of reset) an extra time OR it will
    initialize and enable the WP hardware signal.
    */
    if (ubnt_get_bp() || ubnt_get_srwp()) {
        if (ubnt_get_srwp()) {
            fp_printf("Flash HPM mode will be cleared.\n");
        }
        if (ubnt_get_bp()) {
            fp_printf("Flash BP bits will be cleared.\n");
        }
        ubnt_flash_disable_protection(ubnt_get_srwp());
        /*
        ART u-boot specific.  Restore to initial chip state.
        Disable SRWP and WP line on reboot, if set.
        */
        if (ubnt_get_srwp()) {
            ubnt_flash_hw_write_protect(0);
            ath_spi_write_status(0);
        }
//    } else {
//        printf("Flash protection bits are off.\n  Flash WP signal ignored.")
    }
}
#endif // #ifdef ART_BUILD


#if defined( __KERNEL__)

EXPORT_SYMBOL(ubnt_flash_hw_write_protect);
EXPORT_SYMBOL(ubnt_flash_factory_reset_for_art);
EXPORT_SYMBOL(ubnt_get_srwp);
EXPORT_SYMBOL(ubnt_get_hw_write_protect);
EXPORT_SYMBOL(ubnt_get_bp);
EXPORT_SYMBOL(ubnt_flash_enable_protection);
EXPORT_SYMBOL(ubnt_flash_disable_protection);
EXPORT_SYMBOL(ubnt_flash_print_protection_status);

#endif // #if defined( __KERNEL__)

#endif // #ifdef CONFIG_FLASH_WP

