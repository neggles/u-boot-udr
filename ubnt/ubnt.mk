
#       Copyright Ubiquiti Networks Inc.
#       All Rights Reserved.
#

-include $(UBNT_TOPDIR)/../include/autoconf.mk

CPPFLAGS		:=
CFLAGS			:=
CFLAGS			+= -D__KERNEL__ -D_KERNEL
CFLAGS			+= -Os -msoft-float -pipe -DCONFIG_ARM -D__ARM__ -DBYTE_ORDER=LITTLE_ENDIAN
CFLAGS			+= -Wall -Wstrict-prototypes -fno-stack-protector -Wno-format-nonliteral -mno-unaligned-access
CFLAGS			+= -march=armv7-a -marm -mno-thumb-interwork -mabi=aapcs-linux
CFLAGS			+= -ffunction-sections -fdata-sections -fno-common -ffixed-r9
CFLAGS			+= -mword-relocations -Wno-format-security -fno-stack-protector -fstack-usage -Wno-format-nonliteral -g -Wall -Wstrict-prototypes
CFLAGS			+= -fno-common
CFLAGS			+= -fno-builtin
CFLAGS			+= -ffreestanding
#CFLAGS			+= -nostdinc

#enable this if PEM public key is used for authenticating firmware
#CFLAGS          += -DCONFIG_USE_PEM_PUBLIC_KEY

CFLAGS			+= -I $(UBNT_TOPDIR)/include -I $(UBNT_TOPDIR)/../include -I $(UBNT_TOPDIR)/board/$(VENDOR)/common/ -I$(UBNT_TOPDIR)/../board/$(VENDOR)/common/ -I $(UBNT_TOPDIR)/board/$(VENDOR)/$(BOARD)/ -I $(UBNT_TOPDIR)/crypto/libtommath-0.42.0/ -I $(UBNT_TOPDIR)/crypto/libtomcrypt-1.17/src/headers/
CFLAGS			+= -I $(UBNT_TOPDIR)/../arch/arm/include
#CFLAGS			+= -I $(UBNT_TOPDIR)/../arch/arm/include/asm/arch-mt7622
CROSS_COMPILE	?= arm-linux-
CC			:= $(CROSS_COMPILE)gcc
LD			:= $(CROSS_COMPILE)ld
AR			:= $(CROSS_COMPILE)ar
RANLIB			:= $(CROSS_COMPILE)ranlib
OBJCOPY			:= $(CROSS_COMPILE)objcopy

ifeq ($(DEBUG),1)
ifneq ($(QCA_11AC),1)
# Enable the usetprotect command and also flash_protection debug.
CFLAGS += -DDEBUG=1 -DFP_DEBUG=1
endif
endif

ifeq ($(QCA_11AC),1)
# make a 11AC platform build
CFLAGS += -DQCA_11AC

endif
export CC CFLAGS CROSS_COMPILE LD

include $(UBNT_TOPDIR)/board/$(VENDOR)/$(BOARD)/ubnt.mk
