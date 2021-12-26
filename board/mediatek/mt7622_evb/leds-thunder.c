#include <common.h>
#include <config.h>

#include <asm/arch/mt_gpio.h>
#include "ui-i2c.h"
#include "leds-thunder.h"

static struct thunder_priv {
	int num_leds;
	const struct thunder *cfg;
	int reset_gpio;
	int boot_gpio;
	int address;
} ubnt_thunder_conf = {
	.address = 0x38,
};

struct thunder {
	unsigned int max_leds;
	int isRGB;
	int zeroWidth;
	int oneWidth;
};

static const struct thunder thunder_rgb = {
	.max_leds = 100,
	.isRGB = 0,
	.zeroWidth = 7,
	.oneWidth = 27,
};

static const struct thunder thunder_grb = {
	.max_leds = 100,
	.isRGB = 0, // GRB
	.zeroWidth = 12,
	.oneWidth = 33,
};

static int thunder_read_byte_ack(int address)
{
	uint8_t ret = ui_i2c_read_byte(address);
	if (ret != THUNDER_ACK_OK)
		printf("%s can't get ACK return %x\n", __func__, ret);
	return ret == THUNDER_ACK_OK ? 0 : -1;
}

static int thunder_cmd(int address, THUNDER_CMDS cmd)
{
	int ret = 0;
	uint8_t data[2] = { cmd, 0xFF - cmd };
	ret = ui_i2c_write_data(address, data, 2);
	if (ret)
		printf("CMD %x Error\n", cmd);
	mdelay(1);
	/* TODO: add while-loop for retry  */
	return thunder_read_byte_ack(address);
}

static int thunder_cmd_constant_led(int address, int number,
				    union led_color color)
{
	const struct thunder *cfg = ubnt_thunder_conf.cfg;
	uint8_t data[10];

	if (thunder_cmd(address, I2C_CMD_CONSTANT_LED))
		return -1;
	/* complement */
	data[0] = number;
	data[1] = 0xFF - number;
	/* Color R or G */
	if (cfg->isRGB) {
		data[2] = CODE_R;
		data[3] = color.red;
		data[4] = CODE_G;
		data[5] = color.green;
	} else {
		data[2] = CODE_G;
		data[3] = color.green;
		data[4] = CODE_R;
		data[5] = color.red;
	}
	/* Color B */
	data[6] = CODE_B;
	data[7] = color.blue;
	/* Pulse high width */
	data[8] = cfg->oneWidth;
	/* Pulse low width */
	data[9] = cfg->zeroWidth;

	ui_i2c_write_data(ubnt_thunder_conf.address, data, sizeof(data));
	mdelay(3);
	return thunder_read_byte_ack(ubnt_thunder_conf.address);
}

static int thunder_cmd_breath_led(int address, int number, union led_color cfg)
{
	int speed = cfg.time;
	uint8_t data[10];

	if (thunder_cmd(address, I2C_CMD_BREATH_LED))
		return -1;
	/* complement of number */
	data[0] = number;
	data[1] = 0xFF - number;

	/* Breath speed range from 0~6 */
	if (speed < 0)
		data[2] = 0;
	else if (speed > 6)
		data[2] = 6;
	else
		data[2] = speed;
	data[3] = 0xFF - speed;

	/* Red */
	data[4] = cfg.red;
	data[5] = 0xFF;
	/* Green */
	data[6] = cfg.green;
	data[7] = 0xFF;
	/* Blue */
	data[8] = cfg.blue;
	data[9] = 0xFF;

	ui_i2c_write_data(ubnt_thunder_conf.address, data, sizeof(data));
	mdelay(3);
	return thunder_read_byte_ack(address);
}

int thunder_set_color(union led_color color)
{
	if (color.raw < 0 || color.raw > 0xffffff) {
		printf("wrong color range\n");
		return -1;
	}

	thunder_cmd_constant_led(ubnt_thunder_conf.address,
				 ubnt_thunder_conf.num_leds, color);
	return 0;
}

int thunder_set_breath(union led_color color)
{
	if (color.time < 1 || color.time > 3) {
		printf("wrong speed range 1~3 only\n");
		return -1;
	}
	// The duty_cycle would be calcuated as (x + 1) * 1.2
	// So we have to minus to get the minimun duty_cycle
	color.time -= 1;
	thunder_cmd_breath_led(ubnt_thunder_conf.address,
			       ubnt_thunder_conf.num_leds, color);
	return 0;
}

// Make a write command without data to check if the gd32 mcu exists
int thunder_detect(void) {
	return !ui_i2c_write_data(ubnt_thunder_conf.address, NULL, 0);
}

void thunder_init(void)
{
	static unsigned char is_inited = 0;
	if (is_inited)
		return;

	ubnt_thunder_conf.reset_gpio = CONFIG_UBNT_THUNDER_GPIO_RESET;
	ubnt_thunder_conf.boot_gpio = CONFIG_UBNT_THUNDER_GPIO_BOOT;
	ubnt_thunder_conf.num_leds = CONFIG_UBNT_LED_NUM;
	ubnt_thunder_conf.cfg = &thunder_grb;

	printf("init thunder\n");

	mt_set_gpio_mode(ubnt_thunder_conf.reset_gpio, GPIO_MODE_DEFAULT);
	mt_set_gpio_dir(ubnt_thunder_conf.reset_gpio, GPIO_DIR_OUT); // will pull down
	udelay(1000);
	mt_set_gpio_out(ubnt_thunder_conf.reset_gpio, GPIO_OUT_ONE);
	udelay(100000);

	is_inited = 1;

	// To specify the color order
	thunder_set_color((union led_color){ .raw = 0xffffff });
}
