/*
 * (C) Copyright 2012 Stephen Warren
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __CONFIG_H
#define __CONFIG_H

#include <linux/sizes.h>
#include <configs/autoconf.h>
#include "mt7622_common.h"

#if defined(ON_BOARD_SPI_NOR_FLASH_COMPONENT)

#define CONFIG_DEVICE_NAME udk

#define CONFIG_BOOTCMD_EMMCDUAL_FIT 	\
	"emmcdualfit="						\
		"mtk_ble_init;"						\
		"mmc device 0;" \
		"mmc init;"						\
		"run bootkernel; run bootkernelbkp;"	\
		"run emmcdualfitrecovery; \0" \
	""

#define CONFIG_BOOTCMD_EMMCDUAL_FIT_RECOVERY 	\
	"bootargsrecovery="	\
		"setenv bootargs console=ttyS0,115200 boot=recovery\0" \
	"emmcdualfitrecovery=" 					\
		"mtk_ble_init;"						\
		"mmc device 0;" \
		"mmc init;"						\
		"mmc read $loadaddr 0x00020800 0xffff;"	\
		"run bootargsrecovery;"	\
		"bootm $fitbootconf\0" \
	""
#endif /* ON_BOARD_SPI_NOR_FLASH_COMPONENT */

#ifdef CONFIG_SYS_PROMPT
#undef CONFIG_SYS_PROMPT
#define CONFIG_SYS_PROMPT		            "MT7622 UDK #"
#endif
#endif /* __CONFIG_H */
