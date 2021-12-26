// vim: ts=4:sw=4:expandtab

#include <board_types.h>
#include <ubnt_config.h>
#include <ubnt_types.h>
#include <types.h>
#include <ubnt_board.h>
#include <stdarg.h>
#include <exports.h>
#include <config.h>
#include <ubnt_fw.h>
#include <limits.h>

static ubnt_bdinfo_t ubnt_bdinfo = { .magic = 0 };
#define is_identified(bd) ((bd) && ((bd)->magic == BDINFO_MAGIC))
#define is_not_identified(bd) (!(bd) || ((bd)->magic != BDINFO_MAGIC))
#define mark_identified(bd) do { if ((bd)) { (bd)->magic = BDINFO_MAGIC; } } while (0)

typedef unsigned char enet_addr_t[6];

extern unsigned short be16toh(unsigned short);
extern unsigned short le16toh(unsigned short);

static inline uint16_t
eeprom_get(void* ee, int off) {
	return *(uint16_t*)((char*)ee + off*2);

}

static int
check_eeprom_crc(void *base, void* end, int is_big_endian) {
	uint16_t sum = 0, data, *pHalf;
	uint16_t lengthStruct, i;

	lengthStruct = is_big_endian ? be16toh(*(uint16_t*)base) : le16toh(*(uint16_t*)base);

	if ((((unsigned int)base+lengthStruct) > ((unsigned int)end))
			|| (lengthStruct == 0) ) {
		return 0;
	}

	/* calc */
	pHalf = (uint16_t *)base;

	for (i = 0; i < lengthStruct/2; i++) {
		data = *pHalf++;
        sum ^= is_big_endian ? be16toh(data) : le16toh(data);
	}

	if (sum != 0xFFFF) {
		return 0;
	}

	return 1;
}

typedef struct fw_boardrev_entry {
    unsigned int fw_ver;
    int board_rev_min;
    int board_rev_max;
} fw_boardrev_entry_t;

fw_boardrev_entry_t g_fw_boardrev_uap6pro_table[] = {
    { UAP6PRO_MIN_FW_VERSION, -1, INT_MAX },
    { MAX_FW_VERSION, MAX_BOARD_REV + 1, INT_MAX }
};

fw_boardrev_entry_t *uap_req_revcheck(ubnt_bdinfo_t * bd) {
    fw_boardrev_entry_t *mapping_ptr = NULL;

    if (!bd) return NULL;

    switch (bd->boardid) {
        case SYSTEMID_UAP6PRO:
            mapping_ptr = g_fw_boardrev_uap6pro_table;
            break;
        default:
            mapping_ptr = NULL;
    }

    return mapping_ptr;
}

static unsigned int required_fw_version_by_boardrev(ubnt_bdinfo_t * bd) {
    fw_boardrev_entry_t * mapping_ptr = NULL;

    mapping_ptr = uap_req_revcheck(bd);
    for (   ;
            (mapping_ptr != NULL) && (mapping_ptr->fw_ver != MAX_FW_VERSION);
            mapping_ptr++)  {
        if (    (bd->boardrevision >= mapping_ptr->board_rev_min) &&
                (bd->boardrevision <= mapping_ptr->board_rev_max)    ) {
            return mapping_ptr->fw_ver;
        }
    }

    return MAX_FW_VERSION;
}

static int update_required_fw_version(ubnt_bdinfo_t * bd) {
    unsigned int req_fw_version;

    req_fw_version = required_fw_version_by_boardrev(bd);
    if (req_fw_version > bd->required_fw_version) {
        bd->required_fw_version = req_fw_version;
    }

    return 0;
}

const ubnt_bdinfo_t*
board_identify(ubnt_bdinfo_t* bd) {
	ubnt_bdinfo_t* b = (bd == NULL) ? &ubnt_bdinfo : bd;

    if (is_not_identified(b)) {
        memset(b, 0, sizeof(*b));

        get_boardid(&b->boardid);
        get_vendorid(&b->vendorid);
        get_bomrev(&b->bomrev);
        get_boardrevision(&b->boardrevision);

        update_required_fw_version(b);

        b->cpu_type = BOARD_CPU_TYPE_UNKNOWN;
        b->reset_button_gpio = MT7622_GPIO_BUTTON_RESET;
        b->led_gpio_blue = MT7622_GPIO_LED_BLUE;
        b->led_gpio_white = MT7622_GPIO_LED_WHITE;
        if (b->vendorid == BDINFO_VENDORID_UBNT) {
            switch (b->boardid) {
                case SYSTEMID_UAP6PRO:    /* UAP6-PRO */
                    b->cpu_type = BOARD_CPU_TYPE_MT7622;
                    b->reset_button_gpio = MT7622_GPIO_BUTTON_RESET;
                    b->led_gpio_blue = MT7622_GPIO_LED_BLUE;
                    b->led_gpio_white = MT7622_GPIO_LED_WHITE;
                    break;
            }
        }

        //printf("%s: vendorid=%04x, boardid=%04x, reset_button_gpio=%d\n", __FUNCTION__, b->vendorid, b->boardid, b->reset_button_gpio);

        mark_identified(b);
    }

    return b;
}

void
board_dump_ids(char *ob, const ubnt_bdinfo_t* bd) {

    printf("%04x", bd->boardid);

    if (bd->boardrevision >= 0) {
        printf("-%d", bd->boardrevision);
    }

    printf(".%04x", bd->cpu_revid);

    if ((bd->wifi0.type != WIFI_TYPE_UNKNOWN) && (bd->wifi0.type != WIFI_TYPE_NORADIO)) {
        printf(".%04x", bd->wifi0.type);
    }

    if ((bd->wifi1.type != WIFI_TYPE_UNKNOWN) && (bd->wifi1.type != WIFI_TYPE_NORADIO)) {
        printf(".%04x", bd->wifi1.type);
    }
}

const char*
cpu_type_to_name(int type) {
	switch (type) {
	case BOARD_CPU_TYPE_MT7622:
		return "MT7622";
		break;
	default:
		return "unknown";
	}
	return NULL;
}

