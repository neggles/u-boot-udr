#ifndef LED_COLOR
#define LED_COLOR

#include <linux/types.h>

union led_color {
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
        uint8_t time;
    };
    uint32_t raw;
};

#endif
