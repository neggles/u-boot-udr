#ifndef UI_I2C_H
#define UI_I2C_H

#include <linux/types.h>

uint8_t ui_i2c_read_byte(int address);
int ui_i2c_write_data(int address, uint8_t *data, int len);

#endif
