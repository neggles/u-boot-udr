
/*      Copyright Ubiquiti Networks Inc. 2014.
**      All Rights Reserved.
*/


#ifndef UBNT_MTD__
#define UBNT_MTD__

#define MAX_MTD_PARTS 10

typedef struct mtd_parts {
    char name[24];
    unsigned int size;
    unsigned int offset;
    unsigned int start_address;
    unsigned int start_sector;
    unsigned int end_sector;
} mtd_parts_t;

int mtdparts_init();
int get_mtd_params(char *name, unsigned int *sa, unsigned int *sz,
                   unsigned int *ssect, unsigned int *esect);

#endif /* UBNT_MTD__ */
