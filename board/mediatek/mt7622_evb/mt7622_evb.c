#include <common.h>
#include <config.h>
#include <spi.h>

#include <asm/arch/typedefs.h>
#include <asm/arch/timer.h>
#include <asm/arch/wdt.h>
#include <asm/arch/mt6735.h>
#include <asm/arch/mt_gpio.h>

#include "ui-eeprom.h"
#if defined(CONFIG_MT7622_UDK) || defined(CONFIG_MT7622_UDR)
#include "st7735fb.h"
#endif /* UDK, UDR */
#ifdef CONFIG_MT7622_UDR
#include "leds-ht52241.h"
#include "leds-thunder.h"
#endif

static void board_set_environment(void)
{
#define NOBOOT_CONF_STR "noconf"
	char *lp_env_var = getenv("loadaddr_payload");
	char bootconf_str[128] = NOBOOT_CONF_STR;
	struct eeprom_info *p_udm_board = board_identify();

	if (!p_udm_board) {
		printf("failed to get board info\n");
		return;
	}

	if (p_udm_board->dt_idx == UBNT_UXG_LITE) {
		setenv("board_id", "uxg-lite");
		snprintf(bootconf_str, sizeof(bootconf_str), "%s#%s",
			 lp_env_var, "uxglite@1");
	} else if (p_udm_board->dt_idx == UBNT_UDR) {
		setenv("board_id", "udr");
		snprintf(bootconf_str, sizeof(bootconf_str), "%s#%s",
			 lp_env_var, "udr@1");
	} else if (p_udm_board->dt_idx == UBNT_UDK) {
		setenv("board_id", "udk");
		snprintf(bootconf_str, sizeof(bootconf_str), "%s#%s",
			 lp_env_var, "udk@1");
	} else {
		setenv("board_id", "udm-lite");
		snprintf(bootconf_str, sizeof(bootconf_str), "%s#%s",
			 lp_env_var, "udm-lite@1");
	}

	if (strcmp(bootconf_str, NOBOOT_CONF_STR)) {
		setenv("fitbootconf", bootconf_str);
	}
}

DECLARE_GLOBAL_DATA_PTR;

extern int rt2880_eth_initialize(bd_t *bis);
void soft_spi_gpio_init(void);
/*
 *  Iverson 20140326 : DRAM have been initialized in preloader.
 */

int dram_init(void)
{
    /*
     * UBoot support memory auto detection.
     * So now support both static declaration and auto detection for DRAM size
     */
#if CONFIG_CUSTOMIZE_DRAM_SIZE
    gd->ram_size = CONFIG_SYS_SDRAM_SIZE - SZ_16M;
    printf("static declaration g_total_rank_size = 0x%8X\n", (int)gd->ram_size);
#else
    gd->ram_size = get_ram_size((long *)CONFIG_SYS_SDRAM_BASE,0x80000000) - SZ_16M;
    printf("auto detection g_total_rank_size = 0x%8X\n", (int)gd->ram_size);
#endif

	return 0;
}

int board_init(void)
{
    mtk_timer_init();

#ifdef CONFIG_WATCHDOG_OFF
    mtk_wdt_disable();
#endif

    /* Nelson: address of boot parameters */
	gd->bd->bi_boot_params = CONFIG_SYS_SDRAM_BASE + 0x100;

    return 0;
}

#define RESET_KEY_HOLD_COUNT 1
static void recovery_mode_detect(void)
{
	int count = 0;

	if (mt_set_gpio_mode(GPIO0, GPIO_MODE_01))
		return;

	if (mt_set_gpio_dir(GPIO0, GPIO_DIR_IN))
		return;

	while (!mt_get_gpio_in(GPIO0)) {
		mdelay(1000);
		count += 1;
		if (count < RESET_KEY_HOLD_COUNT) {
			printf("Waiting to recovery mode: %d/%d\n", count, RESET_KEY_HOLD_COUNT);
		} else  {
			setenv("bootcmd", "run emmcdualfitrecovery");
			printf("Entering recovery mode: %d/%d\n", count, RESET_KEY_HOLD_COUNT);
			break;
		}
	}
}

#ifdef CONFIG_MT7622_UDR
void udr_peripherals_init(void)
{
	struct eeprom_info *p_udm_board = board_identify();
	union led_color init_color = { .raw = 0x02ffffff };

	st7735fb_init();
	if (thunder_detect()) {
		thunder_init();
		thunder_set_breath(init_color);
		setenv("leds_mcu", "thunder");
	} else {
		ubnt_ht52241_init();
		ubnt_ht52241_set_breath(init_color);
		setenv("leds_mcu", "ht52241");
	}
}
#endif

#ifdef CONFIG_MT7622_UDK
void udk_peripherals_init(void)
{
	printf("USB 9V\n");
	mt_set_gpio_mode(63, 1);
	mt_set_gpio_mode(64, 1);
	mt_set_gpio_dir(63, 1);
	mt_set_gpio_dir(64, 0);
	mdelay(100);
	mt_set_gpio_out(63, 1);
	st7735fb_init();
}
#endif

void peripherals_init(void)
{
#ifdef CONFIG_MT7622_UDR
	udr_peripherals_init();
#elif defined(CONFIG_MT7622_UDK)
	udk_peripherals_init();
#endif
}

int board_late_init(void)
{
	gd->env_valid = 1; //to load environment variable from persistent store
	env_relocate();
	board_set_environment();
	recovery_mode_detect();
#ifdef CONFIG_SOFT_SPI
	soft_spi_gpio_init();
#endif

	peripherals_init();

	return 0;
}

/*
 *  Iverson todo
 */

void ft_board_setup(void *blob, bd_t *bd)
{
}

#ifndef CONFIG_SYS_DCACHE_OFF
void enable_caches(void)
{
    /* Enable D-cache. I-cache is already enabled in start.S */
    dcache_enable();
}
#endif

int board_eth_init(bd_t *bis)
{
#ifdef CONFIG_RT2880_ETH
    rt2880_eth_initialize(bis);
#endif
    return 0;
}

#ifdef CONFIG_SOFT_SPI
void soft_spi_gpio_init()
{
	mt_set_gpio_mode(MT7622_SPI1_CLK, 1);
	mt_set_gpio_mode(MT7622_SPI1_MOSI, 1);
	mt_set_gpio_mode(MT7622_SPI1_MISO, 1);
	mt_set_gpio_mode(MT7622_SPI1_CS, 1);
	mt_set_gpio_dir(MT7622_SPI1_CLK, 1);
	mt_set_gpio_dir(MT7622_SPI1_MOSI, 1);
	mt_set_gpio_dir(MT7622_SPI1_MISO, 0);
	mt_set_gpio_dir(MT7622_SPI1_CS, 1);
	mt_set_gpio_out(MT7622_SPI1_CS, 1);
}

void spi_scl(int bit)
{
	mt_set_gpio_out(MT7622_SPI1_CLK,bit);
}

void spi_sda(int bit)
{
	mt_set_gpio_out(MT7622_SPI1_MOSI, (bit!=0));
}

unsigned char spi_read(void)
{
	return (unsigned char)mt_get_gpio_in(MT7622_SPI1_MISO);
}

int spi_cs_is_valid(unsigned int bus, unsigned int cs)
{
	return bus == 0 && cs == 0;
}

void spi_cs_activate(struct spi_slave *slave)
{
	mt_set_gpio_out(MT7622_SPI1_CS, 0);
}

void spi_cs_deactivate(struct spi_slave *slave)
{
	mt_set_gpio_out(MT7622_SPI1_CS, 1);
}
#endif
