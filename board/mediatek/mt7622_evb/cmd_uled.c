/*
    Copyright Ubiquiti Networks Inc. 2021
    All Rights Reserved.
*/
#define _STDBOOL_H

#include <common.h>
#include <command.h>
#include <fs.h>
#include "ui-eeprom.h"
#include "leds-ht52241.h"
#include "leds-thunder.h"

static int set_led(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	union led_color color;
	struct eeprom_info *p_udm_board = board_identify();

	if (!p_udm_board) {
		printf("failed to get board info\n");
		return CMD_RET_FAILURE;
	}

	if (argc < 2)
		return CMD_RET_USAGE;

	color.raw = simple_strtoul(argv[1], NULL, 16);

	if (thunder_detect()) {
		thunder_init();
		if (color.raw > 0xffffff)
			thunder_set_breath(color);
		else
			thunder_set_color(color);
	} else {
		ubnt_ht52241_init();
		if (color.raw > 0xffffff)
			ubnt_ht52241_set_breath(color);
		else
			ubnt_ht52241_set_color(color);
	}

	return CMD_RET_SUCCESS;
}

U_BOOT_CMD(uled, CONFIG_SYS_MAXARGS, 1, set_led,
	   "set udr led to booting pattern",
	   "uled <color_hex>\n"
	   "e.g. uled 0x01ff9966");
