
#       Copyright Ubiquiti Networks Inc.
#       All Rights Reserved.
#

include $(UBNT_TOPDIR)/ubnt.mk

LIB	= libucommon.a

SRCS = stubs.c ubnt_app.c ubnt_env.c ubnt_fw.c

ifeq ($(QCA_11AC),1)
SRCS += ubnt_bs.c
endif

ifeq ($(RSA_IMG_SIGNED),1)
SRCS += ubnt_crypto.c
endif

COBJS	= $(patsubst %.c,%.o,$(SRCS))

OBJS	= $(COBJS)

$(LIB):$(OBJS)
	$(LD) $(MIPS_ENDIAN_FLAG) -r -o $@ $(OBJS)

all:	$(COBJS:.o=.c) $(LIB)

clean:
	rm -f $(OBJS) $(LIB)
