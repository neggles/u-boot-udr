
/*      Copyright Ubiquiti Networks Inc. 2014.
**      All Rights Reserved.
*/
#include <common.h>
#include <config.h>
#include <ubnt_mtd.h>
#include <exports.h>

/* mtd string: mtdparts=mt7621-nor0:192k(u-boot),64k(u-boot-env),64k(Factory),64k(EEPROM),7488k(kernel0),7488k(kernel1),1024k(cfg) */

static mtd_parts_t mtd_parts[MAX_MTD_PARTS];
static int total_mtd_nums;
static int mtd_init_done = 0;

/* Find the name within paranthasis */

static char *find_mtd_name(char *str) {
    char *start = str;
    char sbound = 0, ebound = 0;

    while (str && *str != '(')  {
        str++;
        sbound = 1;
    }

    start = ++str;

    while (str && *str != ')')  {
        str++;
        ebound = 1;
    }

    *str = '\0';

    if (ebound && sbound) return start;

    return NULL;
}

/* Find the size that ends with 'k' */

static unsigned long find_mtd_size(char *str) {
    char *last = NULL;
    char *ss = strtok_r(str, "k(", &last);
    int len = strlen(ss);

    ss += strlen(ss);

    while (len-- && ss--) {
        if (isxdigit(*ss))
            continue;
        /* bumb a byte to move to real number */
        ss++;
        break;
    }

    return strtoul(ss, NULL, 10) * 1024;
}


int mtdparts_init() {
    unsigned int i=0, saddr = CFG_FLASH_BASE + 0x60000, size=0, offset=0;
    char *ss, *mtd_str, *tmp;

    mtd_str = getenv("mtdparts");

    if (!mtd_str) {
        mtd_str = MTDPARTS_DEFAULT;
    }

#ifdef DEBUG
       printf("mtd_str=\"%s\"\n", mtd_str);
#endif

    tmp = strdup(mtd_str);
    mtd_str = tmp;

    ss = strtok(mtd_str, ",");

    while (ss) {
        char *name = find_mtd_name(ss);

        if (!name) break;

        strcpy(mtd_parts[i].name, name);

        offset += size;
        /* Add the previous mtd size to get the current start address of
         * the new mtd */
        saddr += size;
        mtd_parts[i].offset =  offset;
        mtd_parts[i].start_address =  saddr;
        mtd_parts[i].start_sector =
                                (saddr - CFG_FLASH_BASE)/CFG_FLASH_SECTOR_SIZE;

        size = find_mtd_size(ss);

        /* Add the current mtd size to get the end sector */
        mtd_parts[i].end_sector =
                   (((saddr + size) - CFG_FLASH_BASE)/CFG_FLASH_SECTOR_SIZE)-1;
        mtd_parts[i++].size =  size;

        ss = strtok(NULL, ",");
    }

    total_mtd_nums = i;

    mtd_init_done = 1;

#ifdef DEBUG
    for (i=0; i < total_mtd_nums; i++)
        printf(" %d. Name = %s, offset = %x, start_addr=%x, size=%d,"
               "start_sector=%d, end_sector=%d \n", i,
               mtd_parts[i].name, mtd_parts[i].offset,
               mtd_parts[i].start_address, mtd_parts[i].size,
               mtd_parts[i].start_sector, mtd_parts[i].end_sector);
#endif

    free(tmp);

    return 0;
}


const mtd_parts_t* find_mtd_by_name(char *name) {
    int i;

    if (!name)
        return NULL;

    if (!mtd_init_done)
        mtdparts_init();

    for (i=0; i < total_mtd_nums; i++) {
        if (!strncmp(name, mtd_parts[i].name, strlen(mtd_parts[i].name)) &&
            strlen(name) == strlen(mtd_parts[i].name)) {
            return (const mtd_parts_t *) &mtd_parts[i];
        }
    }

    return NULL;
}

const unsigned int get_mtd_saddress(char *name) {
    int i;

    if (!name)
        return 0;

    if (!mtd_init_done)
        mtdparts_init();

    for (i=0; i < total_mtd_nums; i++) {
        if (!strncmp(name, mtd_parts[i].name, strlen(mtd_parts[i].name)) &&
            strlen(name) == strlen(mtd_parts[i].name)) {
            return (const unsigned int) mtd_parts[i].start_address;
        }
    }
    return 0;
}

int get_mtd_params(char *name, unsigned int *sa, unsigned int *sz,
                   unsigned int *ssect, unsigned int *esect) {
    int i;

    if (!name)
        return 0;

    if (!mtd_init_done)
        mtdparts_init();

#ifdef DEBUG
    printf("%s: name=%s\n", __func__, name);
#endif

    for (i=0; i < total_mtd_nums; i++) {
        if (!strncmp(name, mtd_parts[i].name, strlen(mtd_parts[i].name)) &&
            strlen(name) == strlen(mtd_parts[i].name)) {
            if (sa) *sa = (unsigned int) mtd_parts[i].start_address;
            if (sz) *sz = (unsigned int) mtd_parts[i].size;
            if (ssect) *ssect = (unsigned int) mtd_parts[i].start_sector;
            if (esect) *esect = (unsigned int) mtd_parts[i].end_sector;
            return 1;
        }
    }
    return 0;
}

const int is_dual_boot() {
    int boot_cnt=0, i=0;

    if (!mtd_init_done)
        mtdparts_init();

#define BOOT_PART_NAME  "kernel"

    for (i=0; i < total_mtd_nums; i++) {
       if (!strncmp(BOOT_PART_NAME, mtd_parts[i].name, strlen(BOOT_PART_NAME)))
       {
          boot_cnt++;
       }
    }

    printf("Number of boot partitions = %d \n", boot_cnt);

    return (boot_cnt >= 2);
}

