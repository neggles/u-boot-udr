    /* Copyright Ubiquiti Networks Inc.
    ** All Rights Reserved.
    */


#ifndef _UBNT_FLASH_H_
#define _UBNT_FLASH_H_

#include <ubnt_config.h>

#ifndef QCA_11AC

#define ATH_GPIO_OE             AR7240_GPIO_OE
#define ATH_GPIO_OUT            AR7240_GPIO_OUT
#define ATH_GPIO_IN             AR7240_GPIO_IN
#define ATH_GPIO_SET            AR7240_GPIO_SET
#define ATH_GPIO_CLEAR          AR7240_GPIO_CLEAR
#define ATH_GPIO_INT_ENABLE     AR7240_GPIO_INT_ENABLE
#define ATH_GPIO_OUT_FUNCTION0 GPIO_OUT_FUNCTION0_ADDRESS
#define ATH_GPIO_OUT_FUNCTION4 GPIO_OUT_FUNCTION4_ADDRESS
#endif

typedef struct flash_details {
    unsigned int flash_id;
    unsigned int base[CFG_MAX_FLASH_SECT];
    unsigned int size;
    unsigned int sector_count;
} flash_uinfo_t;

#define UBNT_FLASH_BASE        0x9f000000
#define UBNT_FLASH_SECTOR_SIZE (8 * 1024 * 1024)
#define UBNT_FLASH_NUM_SECTORS 128

#define KSEG1                   0xa0000000
#define KSEG1ADDR(a) ((__typeof__(a))(((unsigned long )(a) & 0x1fffffff) | KSEG1))

#endif /* _UBNT_FLASH_H_ */

