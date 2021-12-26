
    /* Copyright Ubiquiti Networks Inc.
       All Rights Reserved.
    */

#ifndef __AR9344_H_
#define __AR9344_H_

#include <ar7240_soc.h>

#define CONFIG_UBNTBOOTCOMMAND  "run ubntappinit ubntboot"
#define CONFIG_UBNTAPPINIT      "go ${ubntaddr} uappinit;go ${ubntaddr} ureset_button;urescue;go ${ubntaddr} uwrite"
#define CONFIG_BREV             "go ${ubntaddr} usetbrev"
#define CONFIG_PROTECT          "go ${ubntaddr} usetprotect"
#define CONFIG_BID              "go ${ubntaddr} usetbid"
#define CONFIG_MAC              "go ${ubntaddr} usetmac"
#define CONFIG_RD               "go ${ubntaddr} usetrd"

#define CONFIG_WASP_SUPPORT
#define UBNT_APP                /* to get rid of redefine when compiling */

#endif /* __AR9344_H_ */
