
#       Copyright Ubquiti Networks Inc.
#       All Rights Reserved.
#
SHELL=/bin/bash
UBNT_TOPDIR = $(shell pwd)

export UBNT_TOPDIR BOARD VENDOR LD AR RANLIB

include $(UBNT_TOPDIR)/ubnt.mk

OUTPUT = ubnt_app
UNIFI_KEY_DIR = $(HOME)/unifi-key/uap_mt7621
RSA_SCRAMBLED_KEYFILE = uap_mt7621.pub.b64dec.scrambled
ifeq ($(DEBUG),1)
UBNT_COMBINED_OUTPUT_TMP_FILE=$(UBNT_TOPDIR)/../u-boot-with-app-debug.bin.tmp
UBNT_COMBINED_OUTPUT_FILE=$(UBNT_TOPDIR)/../u-boot-with-app-debug.bin
else
UBNT_COMBINED_OUTPUT_TMP_FILE=$(UBNT_TOPDIR)/../u-boot-with-app.bin.tmp
UBNT_COMBINED_OUTPUT_FILE=$(UBNT_TOPDIR)/../u-boot-with-app.bin
endif

MKIMAGEFLAGS = -T mtkimage \
	-a $(CONFIG_SYS_TEXT_BASE) -e $(CONFIG_SYS_TEXT_BASE) \
	-n "lk=1"

CFLAGS += -I $(CURDIR) -I $(UBNT_TOPDIR)/ubnt/libc/inc
LDFLAGS += -g --gc-sections -T ubnt.lds -Map $(OUTPUT).map

cmd_link_o_target = $(if $(strip $1),\
		      $(LD) -r -o $@ $1,\
		      rm -f $@; $(AR) rcs $@ )

#UBNT_SUBDIRS=libc common $(VENDOR)/$(BOARD) board/$(VENDOR)/common
UBNT_SUBDIRS=libc common board/$(VENDOR)/common
ifeq ($(RSA_IMG_SIGNED),1)
UBNT_SUBDIRS+=builtins crypto/libtomcrypt-1.17 crypto/libtommath-0.42.0
endif

#LIBS := board/$(VENDOR)/$(BOARD)/libuboard.a \
#		board/$(VENDOR)/common/libumtkcommon.a  \
#		common/libucommon.a  \
#		libc/libuc.a
LIBS := board/$(VENDOR)/common/libumtkcommon.a  \
		common/libucommon.a  \
		libc/libuc.a

ifeq ($(RSA_IMG_SIGNED),1)
LIBS+=builtins/libbi.a \
		crypto/libtomcrypt-1.17/libtomcrypt.a \
		crypto/libtommath-0.42.0/libtommath.a
LDFLAGS += -L$(UBNT_TOPDIR)/builtins -lbi
endif

CLEAN_SUFFIX:=.clean
UBNT_SUBDIRS_CLEAN:=$(addsuffix $(CLEAN_SUFFIX),$(UBNT_SUBDIRS))

.PHONY : $(UBNT_SUBDIRS) $(UBNT_SUBDIRS_CLEAN) header

gccdir := $(shell dirname `$(CC) -print-libgcc-file-name`)
ubntlibdir := $(UBNT_TOPDIR)/libc

header:
	@echo "RSA_IMG_SIGNED = $(RSA_IMG_SIGNED)"
	@echo "RSA_KEYFILE = $(RSA_KEYFILE)"
	@echo "Setting up headers"
	echo "#include <config.h>" > $(UBNT_TOPDIR)/include/ubnt_config.h
	echo "#include <configs/$(BOARD).h>" >> $(UBNT_TOPDIR)/include/ubnt_config.h

$(UBNT_SUBDIRS): header
	+$(MAKE) -C $@ all

$(UBNT_SUBDIRS_CLEAN):
	$(MAKE) -C $(basename $@) clean

ifeq ($(RSA_IMG_SIGNED),1)
ubntapp:	header $(UBNT_SUBDIRS)
		$(LD) $(LIBS) $(LDFLAGS) -o $(OUTPUT)
		$(OBJCOPY) -O binary $(UBNT_TOPDIR)/$(OUTPUT) $(UBNT_TOPDIR)/$(OUTPUT).bin  2>/dev/null
		cat $(OUTPUT).bin $(UNIFI_KEY_DIR)/$(RSA_SCRAMBLED_KEYFILE) > $(OUTPUT)-with-key.bin
		cat $(UBNT_TOPDIR)/../u-boot.bin $(OUTPUT)-with-key.bin > $(UBNT_COMBINED_OUTPUT_TMP_FILE)
		(ps=$$(( (4 - ($$(stat -c '%s' $(UNIFI_KEY_DIR)/$(RSA_SCRAMBLED_KEYFILE)) % 4)) % 4)); dd if=/dev/zero bs=1 count=$${ps} >> $(UBNT_COMBINED_OUTPUT_TMP_FILE))
		$(UBNT_TOPDIR)/../tools/mkimage $(MKIMAGEFLAGS) -d $(UBNT_COMBINED_OUTPUT_TMP_FILE) $(UBNT_COMBINED_OUTPUT_FILE) \
			$(if $(KBUILD_VERBOSE:1=), >/dev/null)
else
ubntapp:	$(UBNT_SUBDIRS)
		$(LD) $(LIBS) $(LDFLAGS) -o $(OUTPUT)
		$(OBJCOPY) -O binary $(UBNT_TOPDIR)/$(OUTPUT) $(UBNT_TOPDIR)/$(OUTPUT).bin  2>/dev/null
		cat $(UBNT_TOPDIR)/../u-boot.bin $(OUTPUT).bin > $(UBNT_COMBINED_OUTPUT_TMP_FILE)
		$(UBNT_TOPDIR)/../tools/mkimage $(MKIMAGEFLAGS) -d $(UBNT_COMBINED_OUTPUT_TMP_FILE) $(UBNT_COMBINED_OUTPUT_FILE) \
			$(if $(KBUILD_VERBOSE:1=), >/dev/null)
endif

all: ubntapp

clean: $(UBNT_SUBDIRS_CLEAN)
	-rm -f $(OUTPUT) $(OUTPUT).bin $(OUTPUT)-with-key.bin $(UBNT_COMBINED_OUTPUT_FILE)
	-rm -f $(OUTPUT).map
	-rm -f $(LIBS)
	-rm -f $(UBNT_TOPDIR)/include/ubnt_config.h

