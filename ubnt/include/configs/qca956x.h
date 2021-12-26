
    /* Copyright Ubiquiti Networks Inc. 2014
       All Rights Reserved.
    */

#ifndef __QCA955x_H_
#define __QCA955x_H_

#include <atheros.h>

#define CONFIG_UBNTBOOTCOMMAND  "run ubntappinit; go $ubntaddr ubntboot;bootm $flash_boot_addr"
#define CONFIG_UBNTAPPINIT      "go ${ubntaddr} uappinit;go ${ubntaddr} ureset_button;urescue;go ${ubntaddr} uwrite"
#define CONFIG_BREV             "go ${ubntaddr} usetbrev"
#define CONFIG_PROTECT          "go ${ubntaddr} usetprotect"
#define CONFIG_BID              "go ${ubntaddr} usetbid"
#define CONFIG_MAC              "go ${ubntaddr} usetmac"
#define CONFIG_RD               "go ${ubntaddr} usetrd"
#define MTDPARTS_DEVICE  "ath-nor0"
#define MTDPARTS_INITRAMFS_SIGNED_16    "mtdparts="MTDPARTS_DEVICE":384k(u-boot),64k(u-boot-env),7744k(kernel0),7744k(kernel1),128k(bs),256k(cfg),64k(EEPROM)"


//#define CONFIG_FLASH_WP
//#define FLASH_WP_GPIO    14

#endif /* __QCA956x_H_ */
