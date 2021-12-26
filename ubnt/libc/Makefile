#           Copyright Ubiquiti Networks Inc.
#           All Rights Reserved.
#

include $(UBNT_TOPDIR)/ubnt.mk

LIB	= libuc.a

COBJS	= $(patsubst %.c,%.o,$(wildcard *.c))

SOBJS	= $(patsubst %.S,%.o,$(wildcard *.S))

OBJS	= $(AOBJS) $(COBJS) $(SOBJS)

$(LIB):$(OBJS)
	$(LD) $(MIPS_ENDIAN_FLAG) -r -o $@ $(OBJS)

all:	$(LIB) $(COBJS:.o=.c) $(SOBJS:.o=.S)

clean:
	rm -f $(OBJS) $(LIB)
