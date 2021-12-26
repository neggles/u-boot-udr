/*
 * linux/drivers/video/st7735fb.c -- FB driver for ST7735 LCD controller
 * as found in 1.8" TFT LCD modules available from Adafruit, SainSmart,
 * and other manufacturers.
 * Layout is based on skeletonfb.c by James Simmons and Geert Uytterhoeven.
 *
 * Copyright (C) 2011, Matt Porter
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#include <common.h>
#include <command.h>
#include <malloc.h>
#include <asm/arch/mt_gpio.h>
#include <spi.h>

#include "st7735fb.h"
#include "fb_st7735_ulogo.h"


#if defined(CONFIG_SOFT_SPI)
#define ST7735_SEND_CMD	0x0
#define ST7735_SEND_DATA	0x1

#define MT7622_GPIO_DC		MT7622_LCM_DC
#define MT7622_GPIO_RESET	MT7622_LCM_RESET

#define ST7735_COLSTART 0x18
#define ST7735_ROWSTART 0

/*
   The main image transfer SPI clock speed is set up by the st7735fb_map
   module when it opens our spi_device, but the st7735_cfg_script[] init
   sequence will be limited to this rate:
*/
#define ST7735_SPI_INITCMD_MAX_SPEED	2000000

static struct st7735_function st7735_cfg_script[] = {
#if 1
	{ST7735_START, ST7735_START},
	{ST7735_CMD, ST7735_SWRESET},
	{ST7735_DELAY, 150},
	{ST7735_CMD, ST7735_SLPOUT},
	{ST7735_DELAY, 500},
	{ST7735_CMD, ST7735_FRMCTR1},
	{ST7735_DATA, 0x05},
	{ST7735_DATA, 0x3a},
	{ST7735_DATA, 0x3a},
	{ST7735_CMD, ST7735_FRMCTR2},
	{ST7735_DATA, 0x05},
	{ST7735_DATA, 0x3a},
	{ST7735_DATA, 0x3a},
	{ST7735_CMD, ST7735_FRMCTR3},
	{ST7735_DATA, 0x05},
	{ST7735_DATA, 0x3a},
	{ST7735_DATA, 0x3a},
	{ST7735_DATA, 0x05},
	{ST7735_DATA, 0x3a},
	{ST7735_DATA, 0x3a},
	{ST7735_CMD, ST7735_INVCTR},
	{ST7735_DATA, 0x03},
	{ST7735_DATA, 0x02},
	{ST7735_CMD, ST7735_PWCTR1},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x04},
	{ST7735_CMD, ST7735_PWCTR2},
	{ST7735_DATA, 0xc0},
	{ST7735_CMD, ST7735_PWCTR3},
	{ST7735_DATA, 0x0d},
	{ST7735_DATA, 0x00},
	{ST7735_CMD, ST7735_PWCTR4},
	{ST7735_DATA, 0x8d},
	{ST7735_DATA, 0x2a},
	{ST7735_CMD, ST7735_PWCTR5},
	{ST7735_DATA, 0x8d},
	{ST7735_DATA, 0xee},
	{ST7735_CMD, ST7735_VMCTR1},
	{ST7735_DATA, 0x04},
	/* { ST7735_CMD, ST7735_INVOFF}, */
	{ST7735_CMD, ST7735_MADCTL},
	{ST7735_DATA, 0x08},
	/* { ST7735_CMD, ST7735_CASET}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x4f}, */
	/* { ST7735_CMD, ST7735_RASET}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x00}, */
	/* { ST7735_DATA, 0x9f}, */
	{ST7735_CMD, ST7735_GMCTRP1},
	{ST7735_DATA, 0x10},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x02},
	{ST7735_DATA, 0x03},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x07},
	{ST7735_DATA, 0x02},
	{ST7735_DATA, 0x07},
	{ST7735_DATA, 0x0a},
	{ST7735_DATA, 0x12},
	{ST7735_DATA, 0x27},
	{ST7735_DATA, 0x37},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x0d},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x10},
	{ST7735_CMD, ST7735_GMCTRN1},
	{ST7735_DATA, 0x10},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x03},
	{ST7735_DATA, 0x03},
	{ST7735_DATA, 0x0f},
	{ST7735_DATA, 0x06},
	{ST7735_DATA, 0x02},
	{ST7735_DATA, 0x08},
	{ST7735_DATA, 0x0a},
	{ST7735_DATA, 0x13},
	{ST7735_DATA, 0x26},
	{ST7735_DATA, 0x36},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x0d},
	{ST7735_DATA, 0x0e},
	{ST7735_DATA, 0x10},
	{ST7735_CMD, ST7735_COLMOD},
	{ST7735_DATA, 0x05},
	{ST7735_CMD, ST7735_DISPOFF},
	{ST7735_DELAY, 100},
	/* { ST7735_CMD, ST7735_NORON}, */
	/* { ST7735_DELAY, 10}, */
	{ST7735_END, ST7735_END},
#else
	{ST7735_START, ST7735_START},
	{ST7735_CMD, ST7735_SWRESET},
	{ST7735_DELAY, 150},
	{ST7735_CMD, ST7735_SLPOUT},
	{ST7735_DELAY, 500},
	{ST7735_CMD, ST7735_FRMCTR1},
	{ST7735_DATA, 0x01},
	{ST7735_DATA, 0x2c},
	{ST7735_DATA, 0x2d},
	{ST7735_CMD, ST7735_FRMCTR2},
	{ST7735_DATA, 0x01},
	{ST7735_DATA, 0x2c},
	{ST7735_DATA, 0x2d},
	{ST7735_CMD, ST7735_FRMCTR3},
	{ST7735_DATA, 0x01},
	{ST7735_DATA, 0x2c},
	{ST7735_DATA, 0x2d},
	{ST7735_DATA, 0x01},
	{ST7735_DATA, 0x2c},
	{ST7735_DATA, 0x2d},
	{ST7735_CMD, ST7735_INVCTR},
	{ST7735_DATA, 0x07},
	{ST7735_CMD, ST7735_PWCTR1},
	{ST7735_DATA, 0xa2},
	{ST7735_DATA, 0x02},
	{ST7735_DATA, 0x84},
	{ST7735_CMD, ST7735_PWCTR2},
	{ST7735_DATA, 0xc5},
	{ST7735_CMD, ST7735_PWCTR3},
	{ST7735_DATA, 0x0a},
	{ST7735_DATA, 0x00},
	{ST7735_CMD, ST7735_PWCTR4},
	{ST7735_DATA, 0x8a},
	{ST7735_DATA, 0x2a},
	{ST7735_CMD, ST7735_PWCTR5},
	{ST7735_DATA, 0x8a},
	{ST7735_DATA, 0xee},
	{ST7735_CMD, ST7735_VMCTR1},
	{ST7735_DATA, 0x0e},
	{ST7735_CMD, ST7735_INVOFF},
	{ST7735_CMD, ST7735_MADCTL},
#if ( CONFIG_FB_ST7735_RGB_ORDER_REVERSED == 1 )
	{ST7735_DATA, 0xc0},
#else
	{ST7735_DATA, 0xc8},
#endif
	{ST7735_CMD, ST7735_COLMOD},
	{ST7735_DATA, 0x05},
#if 0				/* set_addr_win will set these, so no need to set them at init */
	{ST7735_CMD, ST7735_CASET},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x00 + ST7735_COLSTART},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, WIDTH - 1 + ST7735_COLSTART},
	{ST7735_CMD, ST7735_RASET},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x00 + ST7735_ROWSTART},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, HEIGHT - 1 + ST7735_ROWSTART},
#endif
	{ST7735_CMD, ST7735_GMCTRP1},
	{ST7735_DATA, 0x02},
	{ST7735_DATA, 0x1c},
	{ST7735_DATA, 0x07},
	{ST7735_DATA, 0x12},
	{ST7735_DATA, 0x37},
	{ST7735_DATA, 0x32},
	{ST7735_DATA, 0x29},
	{ST7735_DATA, 0x2d},
	{ST7735_DATA, 0x29},
	{ST7735_DATA, 0x25},
	{ST7735_DATA, 0x2b},
	{ST7735_DATA, 0x39},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x01},
	{ST7735_DATA, 0x03},
	{ST7735_DATA, 0x10},
	{ST7735_CMD, ST7735_GMCTRN1},
	{ST7735_DATA, 0x03},
	{ST7735_DATA, 0x1d},
	{ST7735_DATA, 0x07},
	{ST7735_DATA, 0x06},
	{ST7735_DATA, 0x2e},
	{ST7735_DATA, 0x2c},
	{ST7735_DATA, 0x29},
	{ST7735_DATA, 0x2d},
	{ST7735_DATA, 0x2e},
	{ST7735_DATA, 0x2e},
	{ST7735_DATA, 0x37},
	{ST7735_DATA, 0x3f},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x00},
	{ST7735_DATA, 0x02},
	{ST7735_DATA, 0x10},
#if 0				/* init_display will turn on the display after clearing it */
	{ST7735_CMD, ST7735_DISPON},
	{ST7735_DELAY, 100},
#endif
	{ST7735_CMD, ST7735_NORON},
	{ST7735_DELAY, 10},
	{ST7735_END, ST7735_END},
#endif
};

static inline void dc_switch(uint8_t cmd){
	if(cmd==ST7735_SEND_CMD)
		mt_set_gpio_out(MT7622_GPIO_DC,ST7735_SEND_CMD);
	else
		mt_set_gpio_out(MT7622_GPIO_DC,ST7735_SEND_DATA);
}



static void st7735_send_byte(struct spi_slave *slave, uint8_t data, uint8_t cmd)
{
	uint8_t buf[1];
	buf[0] = data;
	dc_switch(cmd);
	if(spi_xfer(slave, 8, buf, NULL,
				SPI_XFER_BEGIN | SPI_XFER_END) != 0) {
		printf("Error during SPI transaction\n");
	}
}

static void st7735_send_byte_array(struct spi_slave *slave, uint8_t *data, int len, uint8_t cmd)
{
	int i=1;
	uchar *ch;
	printf("st7735_send_byte_array len= %d \n",len);
	for(i=0; i<len; i++) {
		ch=(data+i);
		st7735_send_byte(slave,*ch,cmd);
	}
}

static void st7735_run_cfg_script(struct spi_slave *slave)
{
	int i = 0;
	int end_script = 0;

	do {
		switch (st7735_cfg_script[i].cmd) {
		case ST7735_START:
			break;
		case ST7735_CMD:
			st7735_send_byte(slave, st7735_cfg_script[i].data & 0xff, ST7735_SEND_CMD);
			break;
		case ST7735_DATA:
			st7735_send_byte(slave, (st7735_cfg_script[i].data & 0xff), ST7735_SEND_DATA);
			break;
		case ST7735_DELAY:
			mdelay(st7735_cfg_script[i].data);
			break;
		case ST7735_END:
			end_script = 1;
		}
		i++;
	} while (!end_script);
}


static int st7735fb_init_display(struct spi_slave *slave)
{
	/* st7735_reset(par); */

	st7735_run_cfg_script(slave);
	mdelay(100);

	return 0;
}



static void st7735fb_gpio_init(void){
	mt_set_gpio_mode(MT7622_GPIO_RESET, 1);
	mt_set_gpio_mode(MT7622_GPIO_DC, 1);
	mt_set_gpio_dir(MT7622_GPIO_RESET, 1);
	mt_set_gpio_dir(MT7622_GPIO_DC, 1);
	mt_set_gpio_out(MT7622_GPIO_RESET,0);
	mt_set_gpio_out(MT7622_GPIO_RESET,1);
	mt_set_gpio_out(MT7622_GPIO_DC,0);
	mt_set_gpio_out(MT7622_GPIO_DC,1);
}

static void st7735fb_draw_logo(struct spi_slave *slave)
{
	uint8_t addr_win_1[4]={0x00, 0x18, 0x00, 0x67};
	uint8_t addr_win_2[4]={0x00, 0x00, 0x00, 0x9f};
	st7735_send_byte(slave, ST7735_CASET & 0xff, ST7735_SEND_CMD);
	udelay(1);
	st7735_send_byte_array(slave, addr_win_1, sizeof(addr_win_1)/sizeof(uchar), ST7735_SEND_DATA);
	udelay(1);
	st7735_send_byte(slave, ST7735_RASET & 0xff, ST7735_SEND_CMD);
	udelay(1);
	st7735_send_byte_array(slave, addr_win_2, sizeof(addr_win_2)/sizeof(uchar), ST7735_SEND_DATA);
	udelay(1);

	st7735_send_byte(slave, ST7735_RAMWR & 0xff, ST7735_SEND_CMD);
	udelay(1);
#if defined(CONFIG_MT7622_UDR)
	st7735_send_byte_array(slave, ulogo_2blue, sizeof(ulogo_2blue)/sizeof(uchar), ST7735_SEND_DATA);
#elif defined(CONFIG_MT7622_UDK)
	st7735_send_byte_array(slave, ulogo_2blue_rotate, sizeof(ulogo_2blue_rotate)/sizeof(uchar), ST7735_SEND_DATA);
#endif
	udelay(1);
	st7735_send_byte(slave, ST7735_DISPON & 0xff, ST7735_SEND_CMD);
}

void st7735fb_init(void)
{
	struct spi_slave *slave = spi_setup_slave(0, 0, 1000000, SPI_MODE_1|SPI_3WIRE);

	spi_claim_bus(slave);
	st7735fb_gpio_init();
	spi_init();
	st7735fb_init_display(slave);
	st7735fb_draw_logo(slave);
	spi_release_bus(slave);
	spi_free_slave(slave);
}

static int do_st7735fb(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	st7735fb_init();
	return 0;
}

U_BOOT_CMD(
	ulogo,	2,	1,	do_st7735fb,
	"show UI logo on LCM st7735fb",
	""
);

#endif
