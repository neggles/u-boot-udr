#ifndef _FLASH_H
#define _FLASH_H

#include <ubnt_flash.h>
#if  !defined(CONFIG_MACH_QCA956x)
#include <ar7240_soc.h>
#else
#include <atheros.h>
#endif

#ifndef QCA_11AC
#define ATH_SPI_FS           0x1f000000
#define ATH_SPI_CLOCK        0x1f000004
#define ATH_SPI_WRITE        0x1f000008
#define ATH_SPI_READ         0x1f000000
#define ATH_SPI_RD_STATUS    0x1f00000c

#define ATH_SPI_CS_DIS       0x70000
#define ATH_SPI_CE_LOW       0x60000
#define ATH_SPI_CE_HIGH      0x60100

#define ATH_SPI_CMD_WRITE_SR     0x01
#define ATH_SPI_CMD_WREN         0x06
#define ATH_SPI_CMD_RD_STATUS    0x05
#define ATH_SPI_CMD_FAST_READ    0x0b
#define ATH_SPI_CMD_PAGE_PROG    0x02
#define ATH_SPI_CMD_SECTOR_ERASE 0xd8
#define ATH_SPI_CMD_CHIP_ERASE   0xc7
#define ATH_SPI_CMD_RDID         0x9f

#define ATH_SPI_SECTOR_SIZE      (1024*64)
#define ATH_SPI_PAGE_SIZE        256
#endif

#define display(_x)     ath_reg_wr_nf(0x18040008, (_x))

/*
 * primitives
 */

#define ath_be_msb(_val, _i) (((_val) & (1 << (7 - _i))) >> (7 - _i))

#define ath_spi_bit_banger(_byte)  do {        \
    int i;                                      \
    for(i = 0; i < 8; i++) {                    \
        ath_reg_wr_nf(ATH_SPI_WRITE,      \
                        ATH_SPI_CE_LOW | ath_be_msb(_byte, i));  \
        ath_reg_wr_nf(ATH_SPI_WRITE,      \
                        ATH_SPI_CE_HIGH | ath_be_msb(_byte, i)); \
    }       \
}while(0);

#define ath_spi_go() do {        \
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CE_LOW); \
    ath_reg_wr_nf(ATH_SPI_WRITE, ATH_SPI_CS_DIS); \
}while(0);


#define ath_spi_send_addr(__a) do {			\
    ath_spi_bit_banger(((__a & 0xff0000) >> 16));	\
    ath_spi_bit_banger(((__a & 0x00ff00) >> 8));	\
    ath_spi_bit_banger(__a & 0x0000ff);		\
} while (0)

#define ath_spi_delay_8()    ath_spi_bit_banger(0)
#define ath_spi_done()       ath_reg_wr_nf(ATH_SPI_FS, 0)

#ifndef QCA_11AC
#define ath_reg_wr_nf        ar7240_reg_wr_nf
#define ath_reg_rd           ar7240_reg_rd
#define ath_reg_rmw_clear    ar7240_reg_rmw_clear
#define ath_reg_wr           ar7240_reg_wr
#define ath_reg_rmw_set      ar7240_reg_rmw_set
#endif

#endif /*_FLASH_H*/
