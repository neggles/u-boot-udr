// vim: ts=4:sw=4:expandtab
/* Copyright Ubiquiti Networks Inc.
** All Right Reserved.
*/

#ifndef _UBNT_EXPORT_H_
#define _UBNT_EXPORT_H_
#include <ubnt_layout.h>

#define UBNT_INVALID_ADDR -1
typedef struct ubnt_app_partition {
    unsigned int addr;
    unsigned int size;
} ubnt_app_partition_t;

typedef struct ubnt_app_update_table {
    ubnt_app_partition_t uboot;
    ubnt_app_partition_t bootselect;
    ubnt_app_partition_t kernel0;
    ubnt_app_partition_t kernel1;
} ubnt_app_update_table_t;

/* this structure shared with uboot common code */
typedef struct ubnt_app_share_data {
    int kernel_index;
    unsigned int kernel_addr;
    ubnt_app_update_table_t update;
} ubnt_app_share_data_t;

enum ubnt_app_events {
    UBNT_EVENT_UNKNOWN          = 0,
    UBNT_EVENT_TFTP_TIMEOUT     = 1,
    UBNT_EVENT_TFTP_RECEIVING   = 2,
    UBNT_EVENT_TFTP_DONE        = 3,
    UBNT_EVENT_ERASING_FLASH    = 4,
    UBNT_EVENT_WRITING_FLASH    = 5,
    UBNT_EVENT_READING_FLASH    = 6,
    UBNT_EVENT_KERNEL_VALID     = 7,
    UBNT_EVENT_KERNEL_INVALID   = 8,
    UBNT_EVENT_END
};

#define UBNT_RAMLOAD_MAGIC 0x4b79ecd3

typedef struct ubnt_ramload {
    unsigned int magic;
    unsigned int size;
    unsigned int xor_m_s;
} ubnt_ramload_t;

typedef union {
    struct ubnt_ramload ramload;
    unsigned int placeholder[16];
} ubnt_ramload_placeholder_t;

extern int ubnt_app_start(int argc, char *const argv[]);
extern int ubnt_is_kernel_loaded(void);
extern void ubnt_app_continue(int event);

#endif
