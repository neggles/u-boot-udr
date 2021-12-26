#include <common.h>
#include "ui-eeprom.h"

extern u32 mtk_nor_read(u32 from, u32 len, size_t *retlen, u_char *buf);

struct eeprom_info *board_identify(void)
{
	static struct eeprom_info udm_board;
	static unsigned char is_cached = 0;
	int ret;
	size_t retlen = 0;

	if (is_cached)
		return &udm_board;

	ret = mtk_nor_read((EEPROM_OFFSET + SUBSYSTEM_ID_OFFSET), 2, &retlen,
			   (u_char *)&udm_board.sys_id);
	if (ret) {
		printf("fetch subsystem ID failed: %d\n", ret);
		return NULL;
	}

	udm_board.sys_id = ntohs(udm_board.sys_id);
	printf("subsystem id: 0x%04x\n", udm_board.sys_id);

	ret = mtk_nor_read((EEPROM_OFFSET + HWREVISION_OFFSET), 4, &retlen,
			   (u_char *)&udm_board.hwrev_id);
	if (ret) {
		printf("fetch hardware revision ID failed: %d\n", ret);
		return NULL;
	}

	udm_board.hwrev_id = ntohl(udm_board.hwrev_id);
	printf("hardware revision id: 0x%08x\n", udm_board.hwrev_id);

	switch (udm_board.sys_id) {
	case SYS_ID_UDM_LITE:
		udm_board.dt_idx = UBNT_UDM_LITE;
		break;
	case SYS_ID_UXG_LITE:
		udm_board.dt_idx = UBNT_UXG_LITE;
		break;
	case SYS_ID_UDR:
		udm_board.dt_idx = UBNT_UDR;
		break;
	case SYS_ID_UDK:
		udm_board.dt_idx = UBNT_UDK;
		break;
	default:
		udm_board.dt_idx = UBNT_UDM_LITE;
		break;
	}

	is_cached = 1;

	return &udm_board;
}
