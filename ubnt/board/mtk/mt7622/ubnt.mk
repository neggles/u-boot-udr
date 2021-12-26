RSA_IMG_SIGNED = 1
RSA_KEYFILE = uap_mt7621.rsa
MIPS_ENDIAN_FLAG = -EL

export RSA_IMG_SIGNED RSA_KEYFILE

override CFLAGS += -fdata-sections -ffunction-sections
ifeq ($(RSA_IMG_SIGNED),1)
override CFLAGS += -DRSA_IMG_SIGNED
endif
