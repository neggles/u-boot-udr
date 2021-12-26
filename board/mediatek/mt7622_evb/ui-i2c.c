#include <common.h>

#include <i2c.h>
#include "ui-i2c.h"

uint8_t ui_i2c_read_byte(int address)
{
	uint8_t buf = 0xff;
	int res = i2c_read(address, 0, 0, &buf, 1);
	if (res) {
		printf("[%s] I2C read error[%d]\n", __func__, res);
	}
	return buf;
}

int ui_i2c_write_data(int address, uint8_t *data, int len)
{
#ifdef DEBUG
	int i;
	printf("write data: ");
	for (i = 0; i < len; i++) {
		printf("%02x ", data[i]);
	}
	printf("\n");
#endif
	return i2c_write(address, 0, 0, data, len);
}
