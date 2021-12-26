#ifndef UI_EEPROM_H
#define UI_EEPROM_H

#include <linux/types.h>

#define EEPROM_OFFSET 0x220000
#define SUBSYSTEM_ID_OFFSET 0x000c
#define HWREVISION_OFFSET 0x0010

#define SYS_ID_UDM_LITE 0xec2d
#define SYS_ID_UXG_LITE 0xb080
#define SYS_ID_UDR 0xeccc
#define SYS_ID_UDK 0xec2e

enum ubnt_udm_dt_idx {
	UBNT_UDM_LITE = 0,
	UBNT_UXG_LITE,
	UBNT_UDR,
	UBNT_UDK,
};

struct eeprom_info {
	uint8_t dt_idx;
	uint16_t sys_id;
	uint32_t hwrev_id;
};

struct eeprom_info *board_identify(void);

#endif
