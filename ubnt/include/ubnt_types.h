// vim: ts=4:sw=4:expandtab

    /* Copyright Ubiquiti Networks Inc.
    ** All Rights Reserved
    */


#ifndef UBNT_TYPES_H_
#define UBNT_TYPES_H_

#include "ubnt_export.h"

#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned int    size_t;
#endif

//typedef unsigned long   IPaddr_t;

    /* Value type retured based on the
       function executed
    */
typedef struct beacon_packet {
    int           ret; /* Note: Overlaps with ret in the union struct below */
    unsigned char *ptr;
    int           len;
} beacon_pkt_t;

typedef union ubnt_exec {
    int ret;
    beacon_pkt_t beacon_pkt;
} ubnt_exec_t;

/* this structure only used by ubnt_app as persistent
 * data between ubnt_app calls */
typedef struct ubnt_app_internal_data {
    unsigned int ubnt_app_state;
    unsigned int led_pattern;
    unsigned int led_current_step;
    unsigned int flags;
} ubnt_app_internal_data_t;

/* this number comes from ubnt.lds */
#define PERSISTENT_STORAGE_SIZE 128
typedef struct ubnt_app_persistent_data {
    ubnt_app_share_data_t share_data;
    ubnt_app_internal_data_t internal_data;
    char util[PERSISTENT_STORAGE_SIZE - sizeof(ubnt_app_share_data_t) - sizeof(ubnt_app_internal_data_t)];
} ubnt_app_persistent_data_t;

#ifndef NULL
#define NULL        0
#endif
#endif
