#ifndef _LEDS_HT52241_H_
#define _LEDS_HT52241_H_

#define SLAVE_ADDR                          0x40
#define STATUS_CHECK_PASS_BYTE              0xaa
#define STATUS_CHECKSUM_CAL_BYTE            0x53
#define MCU_FLASH_WRITE_BUSY_BYTE           0xa1
#define NO_CONFIG_FAIL_BYTE                 0xa6
#define STATUS_CHECK_PASS                   0
#define ERR_STATUS_CHECK_FAIL               1
#define LINE_LIMIT                          0x4B            // line limit cannot exceed 75 (0x4B)
#define BYTES_PER_LED                       4
#define DELAY                               150000          // ns
#define NUM_RETRY                           10
#define IMAGE_LENGTH                        128
#define APP_START_ADDR                      0x1000
#define APP_CRC_SKIP                        0x28
#define IMAGE_SIZE_ADDR_1                   0x24
#define IMAGE_SIZE_ADDR_2                   0x25
#define CRC32_POLYNOMIAL                    0x1EDC6F41
#define CCITT                               (0x1021)

#include "led_color.h"

void ubnt_ht52241_init(void);
int ubnt_ht52241_set_breath(union led_color color);
int ubnt_ht52241_set_color(union led_color color);
int ubnt_ht52241_write_customize(uint8_t *data, int line);

#endif
