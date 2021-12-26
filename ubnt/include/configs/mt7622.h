
/* Copyright Ubiquiti Networks Inc.
   All Rights Reserved.
*/

#ifndef __MT7622_H_
#define __MT7622_H_

#define CONFIG_UBNTBOOTCOMMAND  "run ubntappinit ubntboot"
#define CONFIG_UBNTAPPINIT      "go ${ubntaddr} uappinit;go ${ubntaddr} ureset_button;urescue;go ${ubntaddr} uwrite"
#define CONFIG_BREV             "go ${ubntaddr} usetbrev"
#define CONFIG_PROTECT          "go ${ubntaddr} usetprotect"
#define CONFIG_BID              "go ${ubntaddr} usetbid"
#define CONFIG_MAC              "go ${ubntaddr} usetmac"
#define CONFIG_RD               "go ${ubntaddr} usetrd"

#define UBNT_APP                /* to get rid of redefine when compiling */

#endif /* __MT7621_H_ */
