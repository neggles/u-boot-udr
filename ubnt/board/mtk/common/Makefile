#           Copyright Ubiquiti Networks Inc.
#           All Rights Reserved.
#

include $(UBNT_TOPDIR)/ubnt.mk

LIB	= libumtkcommon.a

SRCS = ubnt_api.c ubnt_board.c ubnt_mtd.c

ifeq ($(ENABLE_FLASH_WP),1)
SRCS += ubnt_flash_protect.c
endif

COBJS	= $(patsubst %.c,%.o,$(SRCS))

OBJS	= $(COBJS)

$(LIB):$(OBJS)
	$(LD) $(MIPS_ENDIAN_FLAG) -r -o $@ $(OBJS)

all:	$(COBJS:.o=.c) $(LIB)

clean:
	rm -f $(OBJS) $(LIB)

