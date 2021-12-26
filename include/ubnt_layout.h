// vim: ts=4:sw=4:expandtab
/* Copyright Ubiquiti Networks Inc.
** All Right Reserved.
*/

#ifndef _UBNT_LAYOUT_H_
#define _UBNT_LAYOUT_H_

#define UBNT_PART_UBOOT_NAME                "u-boot"
#define UBNT_PART_UBOOT_ENV_NAME            "u-boot-env"
#define UBNT_PART_FACTORY_NAME              "Factory"
#define UBNT_PART_EEPROM_NAME               "EEPROM"
#define UBNT_PART_BOOTSELECT_NAME           "bs"
#define UBNT_PART_CFG_NAME                  "cfg"
#define UBNT_PART_KERNEL0_NAME              "kernel0"
#define UBNT_PART_KERNEL1_NAME              "kernel1"

#define KILOBYTES_SHIFT 10

/* UBNT_PART_X_KSIZE are generated in autoconf.h */
#define UBNT_PART_UBOOT_SIZE                (UBNT_PART_UBOOT_KSIZE      << KILOBYTES_SHIFT)
#define UBNT_PART_UBOOT_ENV_SIZE            (UBNT_PART_UBOOT_ENV_KSIZE  << KILOBYTES_SHIFT)
#define UBNT_PART_FACTORY_SIZE              (UBNT_PART_FACTORY_KSIZE    << KILOBYTES_SHIFT)
#define UBNT_PART_EEPROM_SIZE               (UBNT_PART_EEPROM_KSIZE     << KILOBYTES_SHIFT)
#define UBNT_PART_BOOTSELECT_SIZE           (UBNT_PART_BOOTSELECT_KSIZE << KILOBYTES_SHIFT)
#define UBNT_PART_CFG_SIZE                  (UBNT_PART_CFG_KSIZE        << KILOBYTES_SHIFT)
#define UBNT_PART_KERNEL0_SIZE              (UBNT_PART_KERNEL0_KSIZE    << KILOBYTES_SHIFT)
#define UBNT_PART_KERNEL1_SIZE              (UBNT_PART_KERNEL1_KSIZE    << KILOBYTES_SHIFT)

#define UBNT_PART_UBOOT_OFF                 0x60000
#define UBNT_PART_UBOOT_ENV_OFF             (UBNT_PART_UBOOT_OFF        + UBNT_PART_UBOOT_SIZE)
#define UBNT_PART_FACTORY_OFF               (UBNT_PART_UBOOT_ENV_OFF    + UBNT_PART_UBOOT_ENV_SIZE)
#define UBNT_PART_EEPROM_OFF                (UBNT_PART_FACTORY_OFF      + UBNT_PART_FACTORY_SIZE)
#define UBNT_PART_BOOTSELECT_OFF            (UBNT_PART_EEPROM_OFF       + UBNT_PART_EEPROM_SIZE)
#define UBNT_PART_CFG_OFF                   (UBNT_PART_BOOTSELECT_OFF   + UBNT_PART_BOOTSELECT_SIZE)
#define UBNT_PART_KERNEL0_OFF               (UBNT_PART_CFG_OFF          + UBNT_PART_CFG_SIZE)
#define UBNT_PART_KERNEL1_OFF               (UBNT_PART_KERNEL0_OFF      + UBNT_PART_KERNEL0_SIZE)


#define XMK_STR(x)  #x
#define MK_STR(x)   XMK_STR(x)
/**
 * PAREN_INT_TO_STR() - Convert integer macro with parentheses to string
 *
 * The "int" definition in Config.in will be converted to "#define VAR (123)" in
 * "autoconf.h" so the stringify macro needs to eliminate parentheses.
 * E.g. #define VAR  (123) --> "123"
 */
#define PAREN_INT_TO_STR(x) XMK_STR x

#define _mtd_part(name) PAREN_INT_TO_STR(UBNT_PART_##name##_KSIZE)"k("UBNT_PART_##name##_NAME")"

#define MTDPARTS_DEFAULT    "mtdparts=spi1.0:" \
                            _mtd_part(UBOOT)        "," \
                            _mtd_part(UBOOT_ENV)    "," \
                            _mtd_part(FACTORY)      "," \
                            _mtd_part(EEPROM)       "," \
                            _mtd_part(BOOTSELECT)   "," \
                            _mtd_part(CFG)          "," \
                            _mtd_part(KERNEL0)      "," \
                            _mtd_part(KERNEL1)

#endif
