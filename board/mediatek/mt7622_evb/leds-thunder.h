#ifndef LEDS_THUNDER_H
#define LEDS_THUNDER_H

typedef enum {
	I2C_CMD_GET_CMDLIST = 0x00,
	I2C_CMD_GET_VER = 0x01,
	I2C_CMD_GET_PID = 0x06,
	I2C_CMD_READ = 0x11,
	I2C_CMD_GO = 0x21,
	I2C_CMD_WRITE = 0x31,
	I2C_CMD_Ex_EARSE = 0x44,
	I2C_CMD_CONSTANT_LED = 0x50,
	I2C_CMD_CONSTANT_ADDRESS_LED = 0x51,
	I2C_CMD_BREATH_LED = 0x60,
	I2C_CMD_BLINKING_LED = 0x61,
	I2C_CMD_CUSTOM_CONFIG_LED = 0x70,
	I2C_CMD_CUSTOM_RUN_LED = 0x71,
	I2C_CMD_CUSTOM_ADDRESS_LED = 0x72,
	I2C_CMD_DISABLE_PORT_0 = 0xC0,
	I2C_CMD_ENABLE_PORT_0 = 0xD0,
	I2C_CMD_DISABLE_PORT_1 = 0xE0,
	I2C_CMD_ENABLE_PORT_1 = 0xF0,
	I2C_CMD_LOG_EN = 0x99,
	I2C_CMD_LOG_DIS = 0xA9,
	I2C_CMD_LOG_DUMP = 0xB9,
} THUNDER_CMDS;

#define THUNDER_ACK_OK 0x79
#define THUNDER_BLK_READ_CMD 0x0
#define CODE_R 0x52
#define CODE_G 0x47
#define CODE_B 0x42
#define THUNDER_PKG_SIZE 0x20
#define THUNDER_PAGE_SIZE 0x400
#define THUNDER_FW_NAME "thunder_led_fw.bin"
#define THUNDER_PATTERN_NAME "thunder_pattern.bin"
#define THUNDER_FW_SINATURE 0xC01D
#define THUNDER_FW_MAX_SIZE 0x5000
#define THUNDER_BOOTLOADER_MODE 0xB1
#define THUNDER_APP_PAGES 0x10
#define THUNDER_APP_START_PAGE 0x9
#define THUNDER_APP_START_ADDR 0x08002400
#define THUNDER_DEFAULT_WHITE 0xC0C0C0
#define THUNDER_PATTERN_PAGES 0x28
#define THUNDER_PATTERN_START_PAGE 0x18
#define THUNDER_PATTERN_START_ADDR 0x08006000

#include "led_color.h"

int thunder_detect(void);
void thunder_init(void);
int thunder_set_breath(union led_color color);
int thunder_set_color(union led_color color);

#endif // LEDS_THUNDER_H
