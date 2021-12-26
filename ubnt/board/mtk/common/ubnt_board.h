// vim: ts=4:sw=4:expandtab

#ifndef __BOARD_H
#define __BOARD_H

#include <board_types.h>

#define __MAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[3], (a)[4], (a)[5]
#define __MACSTR "%02X:%02X:%02X:%02X:%02X:%02X"

#define BOARD_CPU_TYPE_UNKNOWN -1
#define BOARD_CPU_TYPE_MT7622 0

#define	WIFI_TYPE_UNKNOWN	0xffff
#define WIFI_TYPE_NORADIO   0x0000

#define	HW_ADDR_COUNT 0x01

#define subsysid subdeviceid

#define MT7622_GPIO_BUTTON_RESET    62
#define MT7622_GPIO_LED_WHITE       67
#define MT7622_GPIO_LED_BLUE        68

typedef struct wifi_info {
	unsigned short type;
	unsigned short subvendorid;
	unsigned short subdeviceid;
	unsigned char hwaddr[6];
    unsigned short regdmn;
} wifi_info_t;

typedef struct eth_info {
	unsigned char hwaddr[6];
} eth_info_t;

#define BDINFO_MAGIC 0xDB1FACED

#define BDINFO_VENDORID_UBNT       0x0777 /* from ubnthal_system.h */

typedef struct ubnt_bdinfo {
	unsigned int magic;
	unsigned char cpu_type;
	unsigned short cpu_revid;
	eth_info_t eth0;
	eth_info_t eth1;
    unsigned short boardid;
    unsigned short vendorid;
    unsigned int bomrev;
    int boardrevision;
    wifi_info_t wifi0;
    wifi_info_t wifi1;
    int reset_button_gpio;
    int is_led_gpio_invert;
    int led_gpio_blue;
    int led_gpio_white;
    unsigned int required_fw_version;
} ubnt_bdinfo_t;

const ubnt_bdinfo_t* board_identify(ubnt_bdinfo_t* b);

const char* cpu_type_to_name(int);
void board_dump_ids(char*, const ubnt_bdinfo_t*);

#endif /* __BOARD_H */
