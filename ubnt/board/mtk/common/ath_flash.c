#include <ubnt_config.h>
#include <exports.h>
#include <ath_flash.h>
#include <ubnt_flash.h>
#include <ubnt_board.h>
#include <ubnt_types.h>

#if !defined(ATH_DUAL_FLASH)
#	define	ath_spi_flash_print_info	flash_print_info
#endif

#ifdef UBNT_FLASH_DETECT
typedef struct spi_flash {
	char* name;
	unsigned int jedec_id;
	unsigned int sector_count;
	unsigned int sector_size;
} spi_flash_t;

#define SECTOR_SIZE_64K                 (64*1024)
#define SECTOR_SIZE_128K                (128*1024)
#define SECTOR_SIZE_256K                (256*1024)

static spi_flash_t SUPPORTED_FLASH_LIST[] = {
	/* JEDEC id's for supported flashes ONLY */
	/* ST Microelectronics */
	{ "m25p64",  0x202017,	128, SECTOR_SIZE_64K },  /*  8MB */
	{ "m25p128", 0x202018,	 64, SECTOR_SIZE_256K }, /* 16MB */

	/* Macronix */
	{ "mx25l64",  0xc22017, 128, SECTOR_SIZE_64K },  /*  8MB */
	{ "mx25l128", 0xc22018, 256, SECTOR_SIZE_64K },  /* 16MB */

	/* Spansion */
	{ "s25sl064a", 0x010216, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "s25fl064k", 0x012017, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "s25fl128k", 0x012018, 256, SECTOR_SIZE_64K }, /*  16MB */

	/* Intel */
	{ "s33_64M",    0x898913, 128, SECTOR_SIZE_64K }, /*  8MB */

	/* Winbond */
	{ "w25x64",	0xef3017, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "w25q64",	0xef4017, 128, SECTOR_SIZE_64K }, /*  8MB */
	{ "w25q128",	0xef4018, 256, SECTOR_SIZE_64K }, /*  16MB */

    /* Micron, Numonyx */
    { "n25q064", 0x20ba17, 128, SECTOR_SIZE_64K }, /*  8MB */
    { "n25q128", 0x20ba18, 256, SECTOR_SIZE_64K }, /*  16MB */

    /* EON */
    { "en25qh64", 0x1c7017, 128, SECTOR_SIZE_64K }, /*  8MB */
    { "en25qh128", 0x1c7018, 256, SECTOR_SIZE_64K }, /*  16MB */

};

#define N(a) (sizeof((a)) / sizeof((a)[0]))
#endif /* UBNT_FLASH_DETECT */

/*
 * globals
 */
flash_uinfo_t flash_info[CFG_MAX_FLASH_BANKS];

extern void unifi_set_led(int);
static int display_enabled = 0;

int flash_status_display(int enable) {
	display_enabled = enable;
    if (!display_enabled) {
        unifi_set_led(0);
    }
    return 0;
}

static int led_flip = 0;
static unsigned int led_tick = 0;

static void unifi_toggle_led(unsigned int bound) {
    if (!display_enabled) return;
    led_tick ++;
    if (led_tick > bound) {
        led_flip = (1 - led_flip);
        unifi_set_led(led_flip + 1);
        led_tick %= bound;
    }

}

/*
 * statics
 */
void ath_spi_write_enable(void);
void ath_spi_poll(void);
#if !defined(ATH_SST_FLASH)
static void ath_spi_write_page(unsigned int addr, unsigned char * data, int len);
#endif
static void ath_spi_sector_erase(unsigned int addr);

#ifdef UBNT_FLASH_DETECT
static unsigned int
ath_spi_read_id(unsigned char id_cmd, unsigned char* data, int n)
{
	int i = 0;
	ath_reg_wr_nf(ATH_SPI_FS, 1);
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(id_cmd);
	while (i < n) {
		ath_spi_delay_8();
		data[i] = ath_reg_rd(ATH_SPI_RD_STATUS) & 0xff;
		++i;
	}
	ath_spi_done();
	return 0;
}

static spi_flash_t*
find_flash_data(unsigned int jedec)
{
	spi_flash_t* tmp;
	int i;
	for (i = 0, tmp = &SUPPORTED_FLASH_LIST[0];
			i < N(SUPPORTED_FLASH_LIST);
			i++, tmp++) {
		if (tmp->jedec_id == jedec)
			return tmp;
	}
	return NULL;
}
#else
static void
ath_spi_read_id(void)
{
	unsigned int rd = 0x777777;

	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(ATH_SPI_CMD_RDID);
	ath_spi_delay_8();
	ath_spi_delay_8();
	ath_spi_delay_8();
	ath_spi_go();

	rd = ath_reg_rd(ATH_SPI_RD_STATUS);

	printf("Flash Manuf Id 0x%x, DeviceId0 0x%x, DeviceId1 0x%x\n",
		(rd >> 16) & 0xff, (rd >> 8) & 0xff, (rd >> 0) & 0xff);
}
#endif /* UBNT_FLASH_DETECT */


#ifdef ATH_SST_FLASH
void ath_spi_flash_unblock(void)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_WRITE_SR);
	ath_spi_bit_banger(0x0);
	ath_spi_go();
	ath_spi_poll();
}
#endif

#ifndef CONFIG_ATH_NAND_FL
/*
 * sets up flash_info and returns size of FLASH (bytes)
 */

#define FLASH_M25P64    0x00F2

unsigned long
flash_get_geom (flash_uinfo_t *flash_info)
{
        /* XXX this is hardcoded until we figure out how to read flash id */

        flash_info->flash_id = FLASH_M25P64;
        flash_info->size = CFG_FLASH_SIZE; /* bytes */
        flash_info->sector_count = flash_info->size / CFG_FLASH_SECTOR_SIZE;

#ifndef UBNT_FLASH_DETECT
        for (i = 0; i < flash_info->sector_count; i++) {
                flash_info->base[i] = CFG_FLASH_BASE +
                                        (i * CFG_FLASH_SECTOR_SIZE);
        }
#endif
        printf ("flash size %dMB, sector count = %d\n",
                        FLASH_SIZE, flash_info->sector_count);

        return (flash_info->size);
}
#endif /* CONFIG_ATH_NAND_FL */

#ifdef UBNT_FLASH_DETECT
unsigned long flash_init(void)
{
	unsigned char idbuf[5];
	unsigned int jedec;
	spi_flash_t* f;
	flash_uinfo_t* flinfo = &flash_info[0];
	int i;
	unsigned int sector_size = 0;

#ifndef CONFIG_WASP
#ifdef ATH_SST_FLASH
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x3);
	ath_spi_flash_unblock();
	ath_reg_wr(ATH_SPI_FS, 0);
#else
#ifdef CONFIG_MACH_QCA956x
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x246);
#else
	ath_reg_wr_nf(ATH_SPI_CLOCK, 0x43);
#endif
#endif
#endif
	ath_spi_read_id(0x9f, idbuf, 3);
	jedec = (idbuf[0] << 16) | (idbuf[1] << 8) | idbuf[2];

	f = find_flash_data(jedec);
	if (f) {
		flinfo->flash_id = jedec;
		flinfo->size = f->sector_count * f->sector_size;
		flinfo->sector_count = f->sector_count;
	} else {
		/* flash was not detected - take the one, hardcoded
		 * in board info */
        printf("Unrecognized flash 0x%06x\n", jedec);
        flash_get_geom(flinfo);
	}

	sector_size = flinfo->size / flinfo->sector_count;

	/* fill in sector info */
	for (i = 0; i < flinfo->sector_count; ++i) {
		flinfo->base[i] = CFG_FLASH_BASE + (i * sector_size);
	}

	return flinfo->size;
}

void
ath_spi_flash_print_info(flash_uinfo_t *info)
{
	spi_flash_t* f;
	f = find_flash_data(info->flash_id);
	if (f) {
		printf("%s ", f->name);
	}
	printf("(Id: 0x%06x)\n", info->flash_id);
	printf("\tSize: %ld MB in %d sectors\n",
			info->size >> 20,
			info->sector_count);
}

int
flash_erase(flash_uinfo_t *info, int s_first, int s_last)
{
    int i, sector_size = info->size / info->sector_count;

    if (ubnt_disable_protection())
        return -1;

    printf("\nFirst %#x last %#x sector size %#x\n",
           s_first, s_last, sector_size);

    for (i = s_first; i <= s_last; i++) {
        printf(".");
        ath_spi_sector_erase(i * sector_size);
    }
    ath_spi_done();
    printf(" done\n");

    if (ubnt_enable_protection())
        return -1;

    return 0;
}

#else
int
flash_erase(flash_uinfo_t *info, int s_first, int s_last)
{
    int i, sector_size = info->size / info->sector_count;

    if (ubnt_disable_protection())
        return -1;

    printf("\nFirst %#x last %#x sector size %#x\n",
            s_first, s_last, sector_size);

    for (i = s_first; i <= s_last; i++) {
         printf("\b\b\b\b%4d", i);
         ath_spi_sector_erase(i * sector_size);
    }

    ath_spi_done();
    printf("\n");

    if (ubnt_enable_protection())
        return -1;

    return 0;
}
#endif /* UBNT_FLASH_DETECT */

/*
 * Write a buffer from memory to flash:
 * 0. Assumption: Caller has already erased the appropriate sectors.
 * 1. call page programming for every 256 bytes
 */
#ifdef ATH_SST_FLASH
void
ath_spi_flash_chip_erase(void)
{
    ath_spi_write_enable();
    ath_spi_bit_banger(ATH_SPI_CMD_CHIP_ERASE);
    ath_spi_go();
    ath_spi_poll();
}

int
write_buff(flash_uinfo_t *info, unsigned char *src, unsigned long dst, unsigned long len)
{
    unsigned int val;

    if (ubnt_disable_protection())
        return -1;

    dst = dst - CFG_FLASH_BASE;

    for (; len; len--, dst++, src++) {
        ath_spi_write_enable();	// dont move this above 'for'
        ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
        ath_spi_send_addr(dst);

        val = *src & 0xff;
        ath_spi_bit_banger(val);

        ath_spi_go();
        ath_spi_poll();
    }

    /*
     * Disable the Function Select
     * Without this we can't read from the chip again
     */

    ath_reg_wr(ATH_SPI_FS, 0);

    if (ubnt_enable_protection())
        return -1;

    if (len) {
	// how to differentiate errors ??
	return -1;
    } else {
        return 0;
    }
}
#else
int
write_buff(flash_uinfo_t *info, unsigned char *source, unsigned long addr, unsigned long len)
{
    int total = 0, len_this_lp, bytes_this_page;
    unsigned long dst;
    unsigned char *src;

    if (ubnt_disable_protection())
        return -1;

    printf("write addr: %x\n", addr);
    addr = addr - CFG_FLASH_BASE;

    while (total < len) {
        src = source + total;
        dst = addr + total;
        bytes_this_page =
        ATH_SPI_PAGE_SIZE - (addr % ATH_SPI_PAGE_SIZE);
        len_this_lp = ((len - total) >
                    bytes_this_page) ? bytes_this_page : (len - total);
        ath_spi_write_page(dst, src, len_this_lp);
        total += len_this_lp;
    }

    ath_spi_done();

    if (ubnt_enable_protection())
        return -1;

    return 0;
}
#endif

void
ath_spi_write_enable()
{
	ath_reg_wr_nf(ATH_SPI_FS, 1);
	ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
	ath_spi_bit_banger(ATH_SPI_CMD_WREN);
	ath_spi_go();
}

void
ath_spi_poll()
{
	int rd;

	do {
		ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS);
		ath_spi_bit_banger(ATH_SPI_CMD_RD_STATUS);
		ath_spi_delay_8();
		rd = (ath_reg_rd(ATH_SPI_RD_STATUS) & 1);
	} while (rd);
}

#if !defined(ATH_SST_FLASH)
static void
ath_spi_write_page(unsigned int addr, unsigned char *data, int len)
{
	int i;
	unsigned char ch;

	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_PAGE_PROG);
	ath_spi_send_addr(addr);

	for (i = 0; i < len; i++) {
		ch = *(data + i);
		ath_spi_bit_banger(ch);
	}

	ath_spi_go();
	ath_spi_poll();
    unifi_toggle_led(512);
}
#endif

static void
ath_spi_sector_erase(unsigned int addr)
{
	ath_spi_write_enable();
	ath_spi_bit_banger(ATH_SPI_CMD_SECTOR_ERASE);
	ath_spi_send_addr(addr);
	ath_spi_go();
#ifdef CONFIG_WASP_SUPPORT
	unifi_toggle_led(4);
#else
        unifi_toggle_led(1);
#endif
	ath_spi_poll();
}
