/*
 * linux/include/video/st7735fb.h -- FB driver for ST7735 LCD controller
 * as found in 1.8" TFT LCD modules available from Adafruit, SainSmart,
 * and other manufacturers.
 *
 * Copyright (C) 2011, Matt Porter
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */

#define DRVNAME		"st7735fb"
#define WIDTH		80
#define HEIGHT		160
#define BPP		16

/* Supported display modules */
#define ST7735_DISPLAY_COMMON_TFT18	0	/* common 1.8" TFT LCD */

/* Init script function */
struct st7735_function {
	uint16_t cmd;
	uint16_t data;
};

/* Init script commands */
enum st7735_cmd {
	ST7735_START,
	ST7735_END,
	ST7735_CMD,
	ST7735_DATA,
	ST7735_DELAY
};

struct st7735fb_par {
	struct spi_device *spi;
	struct fb_info *info;
	u8 *spi_writebuf;
	u32 pseudo_palette[16];
	int rst;
	int dc;
	struct {
		int xs;
		int xe;
		int ys;
		int ye;
	} addr_win;
	volatile long unsigned int deferred_pages_mask;
};

struct st7735fb_platform_data {
	int rst_gpio;
	int dc_gpio;
};

/* ST7735 Commands */
#define ST7735_NOP	0x0
#define ST7735_SWRESET	0x01
#define ST7735_RDDID	0x04
#define ST7735_RDDST	0x09
#define ST7735_SLPIN	0x10
#define ST7735_SLPOUT	0x11
#define ST7735_PTLON	0x12
#define ST7735_NORON	0x13
#define ST7735_INVOFF	0x20
#define ST7735_INVON	0x21
#define ST7735_DISPOFF	0x28
#define ST7735_DISPON	0x29
#define ST7735_CASET	0x2A
#define ST7735_RASET	0x2B
#define ST7735_RAMWR	0x2C
#define ST7735_RAMRD	0x2E
#define ST7735_COLMOD	0x3A
#define ST7735_MADCTL	0x36
#define ST7735_FRMCTR1	0xB1
#define ST7735_FRMCTR2	0xB2
#define ST7735_FRMCTR3	0xB3
#define ST7735_INVCTR	0xB4
#define ST7735_DISSET5	0xB6
#define ST7735_PWCTR1	0xC0
#define ST7735_PWCTR2	0xC1
#define ST7735_PWCTR3	0xC2
#define ST7735_PWCTR4	0xC3
#define ST7735_PWCTR5	0xC4
#define ST7735_VMCTR1	0xC5
#define ST7735_RDID1	0xDA
#define ST7735_RDID2	0xDB
#define ST7735_RDID3	0xDC
#define ST7735_RDID4	0xDD
#define ST7735_GMCTRP1	0xE0
#define ST7735_GMCTRN1	0xE1
#define ST7735_PWCTR6	0xFC

void st7735fb_init(void);
