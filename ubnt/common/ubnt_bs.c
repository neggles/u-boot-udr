
    /* Copyright (C) Ubiquiti Networks Inc. 2014.
    ** All Rights Reserved.
    */

#include <config.h>
#include <ubnt_types.h>

#define KILOBYTES_SHIFT             10

#define UBNT_PART_BOOTSEL_SIZE      64

#define UBNT_PART_BOOTSEL_SIZE_H    (UBNT_PART_BOOTSEL_SIZE << KILOBYTES_SHIFT)

#define UBNT_BS_MAGIC 0xA34DE82B

//extern int ubnt_flash_read(unsigned char *, unsigned char *);
//extern int ubnt_flash_update(unsigned char*, unsigned char *, size_t);

typedef struct ubnt_bootsel {
    unsigned int bootsel : 1;
    /* Due to backward compatibility reasons magic is placed in 2nd place */
    unsigned int magic;
    unsigned char pad[UBNT_PART_BOOTSEL_SIZE_H - 8];
    /* Fill any new parameters here */
} ubnt_bs_t;

static ubnt_bs_t bootsel;

/* Init the bootsel sector if its invalid */
int ubnt_bootsel_init() {
#if 0
    int rc = 0;

    if ((rc = ubnt_flash_read("bs", (unsigned char *) &bootsel))) {
        printf(" Flash read failed \n");
        return rc;
    }

    if (bootsel.magic != UBNT_BS_MAGIC) {
        printf("Warning:Bootsel magic mismatch:%x, setting to correct value \n",
               bootsel.magic);

        memset(&bootsel, 0, sizeof(bootsel));

#define ART_DEFAULT 0xffffffff

        /* If both the boot selection word and magic word are 0xffffffff
           then its an upgrade from ART build.
        */
        if (bootsel.magic == ART_DEFAULT &&
            *(unsigned int *) &bootsel == ART_DEFAULT) {
            printf("ART build. Setting the default for boot selection \n");
            bootsel.bootsel = 0;
        }

        bootsel.magic = UBNT_BS_MAGIC;

        rc = ubnt_flash_update("bs",(unsigned char *) &bootsel,
                               UBNT_PART_BOOTSEL_SIZE_H);
    }

    printf("%s: bootsel magic=%x, bootsel = %x \n", __func__, bootsel.magic,
            bootsel.bootsel);
    return rc;
#endif
    return 0;
}


/* Get the current default kernel boot selection */
int ubnt_get_bootsel(unsigned int *bs) {
#if 0
    int rc=0;

    if ((rc = ubnt_flash_read("bs", (unsigned char *) &bootsel))) {
        printf(" Flash read failed \n");
        *bs = 0;
        return rc;
    }

    if (bootsel.magic != UBNT_BS_MAGIC) {
        printf("WARNING:Boot selection logic magic mismatch\n");
        /* Return kernel 0  partition */
        *bs = 0;
        return -1;
    }

    printf("%s: Boot partition selected = %x \n", __func__, bootsel.bootsel);
    *bs = bootsel.bootsel;

    return rc;
#endif
    return 0;
}


/* Sets the default boot selection number. 0 - kernel-0, 1 - kernel-1 */
int ubnt_set_bootsel(int num) {
#if 0
    int rc=0;

    if ((rc = ubnt_flash_read("bs", (unsigned char *) &bootsel))) {
        printf(" Flash read failed \n");
        return rc;
    }

    if (bootsel.magic != UBNT_BS_MAGIC) {
        printf("WARNING:Boot selection logic magic mismatch, resetting\n");
        rc = ubnt_bootsel_init();
    }

    bootsel.bootsel = num & 0x1;

    rc = ubnt_flash_update("bs", (unsigned char *)&bootsel,
                           UBNT_PART_BOOTSEL_SIZE_H);

    return rc;
#endif
    return 0;
}
