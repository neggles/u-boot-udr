#           Copyright Ubiquiti Networks Inc.
#           All Rights Reserved.
#

include $(UBNT_TOPDIR)/ubnt.mk

LIB	= libbi.a

#COBJS	= $(patsubst %.c,%.o,$(wildcard *.c))
#CSRCS	= udivmoddi4.c udivmodsi4.c udivsi3.c
CSRCS	= udivdi3.c udivmoddi4.c arm/__aeabi_idiv0.c
COBJS	= $(patsubst %.c,%.o,$(CSRCS))
SSRCS	= arm/aeabi_uldivmod.S arm/aeabi_idivmod.S arm/divsi3.S arm/aeabi_uidivmod.S arm/udivsi3.S arm/divmodsi4.S arm/udivmodsi4.S
SOBJS	= $(patsubst %.S,%.o,$(SSRCS))

OBJS	= $(COBJS) $(SOBJS)

$(LIB):$(OBJS)
	$(AR) crv $@ $(OBJS)

all:	$(LIB) $(CSRCS) $(SRCS)

clean:
	rm -f $(OBJS) $(LIB)
