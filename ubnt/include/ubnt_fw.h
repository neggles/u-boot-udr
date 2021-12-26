// vim: ts=4:sw=4:expandtab

/*          Copyright Ubiquiti Networks Inc. 2016.
**          All Rights Reserved.
*/
#ifndef UBNT_FW_H
#define UBNT_FW_H

enum {
    HEADER_FIELD_SIZE_VERSION                       = 256,
};

typedef struct header {
    char magic[4];
    char version[HEADER_FIELD_SIZE_VERSION];
    unsigned long crc;
    unsigned long pad;
} __attribute__ ((packed)) header_t;

typedef struct part {
    char magic[4];
    char name[16];
	char pad[12];
	unsigned long mem_addr;
    unsigned long id;
    unsigned long base_addr;
    unsigned long entry_addr;
    unsigned long length;
    unsigned long part_size;
} __attribute__ ((packed)) part_t;

typedef struct part_crc {
    unsigned long crc;
    unsigned long pad;
} __attribute__ ((packed)) part_crc_t;

typedef struct signature {
    char magic[4];
#ifndef RSA_IMG_SIGNED
    unsigned long crc;
#else
    unsigned long rsa_signed_hash[64];
#endif
    unsigned long pad;
} __attribute__ ((packed)) signature_t;


typedef struct partition_data {
	part_t* header;
	unsigned char* data;
	part_crc_t* signature;
	unsigned int valid;
} __attribute__ ((packed)) partition_data_t;

#define FWTYPE_UNKNOWN	0
#define FWTYPE_UBNT	1
#define FWTYPE_OPEN	2
#define FWTYPE_OLD	3
#define FWTYPE_MAX	4

//extern flash_uinfo_t flash_info[CFG_MAX_FLASH_BANKS];

#define MAX_PARTITIONS 8

typedef struct fw {
	unsigned char version[128];
	unsigned char type;
	partition_data_t parts[MAX_PARTITIONS];
	unsigned int part_count;
} fw_t;

// fw_check_header() possible return values
enum {
    E_SUCCESS                                   = 0,
    E_TYPE_NOT_MATCH                            = 1,
    E_CHECKSUM_FAILED                           = 2,
    E_ARCH_NOT_MATCH                            = 3,
    E_VERSION_CHECK_FAILED                      = 4,
    E_VERSION_STRING_CHECK_FAILED               = 5,
};

#define FW_VERSION_STRING_DELIMITER                "."

// struct for parsing version field in header_t
typedef struct fw_version_string_info {
    char version[HEADER_FIELD_SIZE_VERSION];
    char *arch;
    char *p_major;
    char *p_minor;
    char *p_update;
    char *remaining;
    unsigned int is_fw_version_no_valid;    // major, minor and update are invalid if this is 0
    int major;
    int minor;
    int update;
} fw_version_string_info_t;

fw_t * ubnt_fw_get_info(void);
int ubnt_fw_check(void* base, int size);

#define MK_FW_VERSION(major, minor, update) (((major & 0xff) << 16) | ((minor & 0xff) << 8) | (update & 0xff))

#define MIN_FW_VERSION  MK_FW_VERSION(0, 0, 0)
#define MAX_FW_VERSION  MK_FW_VERSION(0xff, 0xff, 0xff)

#define GET_FW_MAJOR_VERSION(fw_ver)        ((fw_ver >> 16) & 0xff)
#define GET_FW_MINOR_VERSION(fw_ver)        ((fw_ver >>  8) & 0xff)
#define GET_FW_UPDATE_VERSION(fw_ver)       (fw_ver & 0xff)

#define MAX_BOARD_REV 0xff  /* 1 byte long */

#define SYSTEMID_UAP6PRO        0xa620

#define UAP6PRO_MIN_FW_VERSION                  MK_FW_VERSION(9, 9, 9) // TODO fill this in properly when first HDIW version is tagged

#endif
