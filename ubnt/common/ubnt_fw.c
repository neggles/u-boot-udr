// vim: ts=4:sw=4:expandtab
/*          Copyright Ubiquiti Networks Inc. 2014.
**          All Rights Reserved.
*/

#include <ubnt_config.h>
#include <common.h>
#include <exports.h>
#include <ubnt_mtd.h>
#include <config.h>
#include <ubnt_fw.h>
#include <ubnt_crypto.h>
#include <limits.h>
#include <errno.h>
//#include <string.h>
//#include <ath_flash.h>

#ifdef REQUIRED_RECOVERY_FW_VERSION
#include <ubnt_board.h>
#endif

#ifdef RSA_IMG_SIGNED
// scrambled
static const unsigned char UAP_MT7621_REVOKED_FW_HASH[] = {
};
#endif

/*
 *  Ubiquiti firmware stuff
 */

unsigned char get_fw_type(const header_t* h)
{
    if (strncmp(h->magic, "UBNT", 4) == 0) {
        return FWTYPE_UBNT;
    } else if (strncmp(h->magic, "GEOS", 4) == 0) {
        return FWTYPE_OLD;
    } else if (strncmp(h->magic, "OPEN", 4) == 0) {
        return FWTYPE_OPEN;
    } else {
        return FWTYPE_UNKNOWN;
    }
}

int is_fw_type_valid(unsigned char type)
{
    if (type > FWTYPE_UNKNOWN && type < FWTYPE_MAX) {
        return 1;
    } else {
        return 0;
    }
}

int is_checksum_valid(const header_t* h)
{
    unsigned long crc;
    unsigned int len = sizeof(header_t) - 2 * sizeof(unsigned long);

    crc = ucrc32((const unsigned char*)h, len);
    if (htonl(crc) == h->crc) {
        return 1;
    } else {
        return 0;
    }
}

#ifdef RECOVERY_FWVERSION_ARCH
int is_fw_version_string_valid(const header_t* h)
{
    int len = strnlen(h->version, HEADER_FIELD_SIZE_VERSION);

    if (len >= HEADER_FIELD_SIZE_VERSION) {
        return 0;
    } else {
        return 1;
    }
}

#ifdef __DEBUG2
void dump_fw_version_string_info(const fw_version_string_info_t *p_fw_version_string_info)
{
    if (p_fw_version_string_info == NULL) {
        return;
    }

    printf("version: \"%s\"\n", p_fw_version_string_info->version ? p_fw_version_string_info->version : "<NULL>");
    printf("arch: \"%s\"\n", p_fw_version_string_info->arch ? p_fw_version_string_info->arch : "<NULL>");
    printf("p_major: \"%s\"\n", p_fw_version_string_info->p_major ? p_fw_version_string_info->p_major : "<NULL>");
    printf("p_minor: \"%s\"\n", p_fw_version_string_info->p_minor ? p_fw_version_string_info->p_minor : "<NULL>");
    printf("p_update: \"%s\"\n", p_fw_version_string_info->p_update ? p_fw_version_string_info->p_update : "<NULL>");
    printf("remaining: \"%s\"\n", p_fw_version_string_info->remaining ? p_fw_version_string_info->remaining : "<NULL>");
    printf("is_fw_version_no_valid: %d\n", p_fw_version_string_info->is_fw_version_no_valid);
    printf("major: %d\n", p_fw_version_string_info->major);
    printf("minor: %d\n", p_fw_version_string_info->minor);
    printf("update: %d\n", p_fw_version_string_info->update);
}
#endif

extern int errno;

int parse_version_no(const char *value_in_string, int *p_value)
{
    char *endptr;
    unsigned long value = 0;

    errno = 0;
    value = strtoul(value_in_string, &endptr, 10);
    if (0
        || value_in_string == endptr
        || *endptr != '\0'
        || (value == LONG_MAX && errno == ERANGE)
        ) {
        return -1;
    } else {
        *p_value = (int) value;
        return 0;
    }
}

int parse_fw_version_string(
    const header_t *h,
    fw_version_string_info_t *p_fw_version_string_info
    )
{
    char *str, *token;
    const char *delimiter = FW_VERSION_STRING_DELIMITER;

    if (p_fw_version_string_info == NULL) {
        return -1;
    }

    // h->version must be checked for NULL termination first
    strcpy(p_fw_version_string_info->version, h->version);

    str = p_fw_version_string_info->version;

    // version string example:
    // BZ.ipq806x.v3.8.7.6681.170720.2049
    // BZ.ipq806x.feature-spf4.7062.170725.1642

    token = strsep(&str, delimiter);        // the BZ part
    p_fw_version_string_info->arch = strsep(&str, delimiter);

    // str is now:
    // v3.8.7.6681.170720.2049
    if (str[0] == 'v') {
        str++;
        p_fw_version_string_info->p_major = strsep(&str, delimiter);
        p_fw_version_string_info->p_minor = strsep(&str, delimiter);
        p_fw_version_string_info->p_update = strsep(&str, delimiter);
    } else {
        p_fw_version_string_info->p_major = NULL;
        p_fw_version_string_info->p_minor = NULL;
        p_fw_version_string_info->p_update = NULL;
    }

    if (1
        && p_fw_version_string_info->p_major != NULL
        && p_fw_version_string_info->p_minor != NULL
        && p_fw_version_string_info->p_update != NULL
        ) {
        p_fw_version_string_info->is_fw_version_no_valid =
            parse_version_no(
                p_fw_version_string_info->p_major,
                &p_fw_version_string_info->major
                ) == 0;

        if (p_fw_version_string_info->is_fw_version_no_valid) {
            p_fw_version_string_info->is_fw_version_no_valid =
                parse_version_no(
                    p_fw_version_string_info->p_minor,
                    &p_fw_version_string_info->minor
                    ) == 0;
        }

        if (p_fw_version_string_info->is_fw_version_no_valid) {
            p_fw_version_string_info->is_fw_version_no_valid =
                parse_version_no(
                    p_fw_version_string_info->p_update,
                    &p_fw_version_string_info->update
                    ) == 0;
        }
    } else {
        p_fw_version_string_info->is_fw_version_no_valid = 0;
    }

    if (!p_fw_version_string_info->is_fw_version_no_valid) {
        p_fw_version_string_info->major = 0;
        p_fw_version_string_info->minor = 0;
        p_fw_version_string_info->update = 0;
    }

    p_fw_version_string_info->remaining = str;

#ifdef __DEBUG2
    dump_fw_version_string_info(p_fw_version_string_info);
#endif

    return 0;
}

int is_arch_valid(const fw_version_string_info_t *p_fw_version_string_info)
{
    if (p_fw_version_string_info == NULL) {
        return 0;
    }

    if (strcmp(p_fw_version_string_info->arch, RECOVERY_FWVERSION_ARCH)) {
        return 0;
    } else {
        return 1;
    }
}

#ifdef REQUIRED_RECOVERY_FW_VERSION
int pass_fw_version_check(const fw_version_string_info_t *p_fw_version_string_info)
{
    ubnt_bdinfo_t *bd = NULL;

#ifdef SKIP_FW_VERSION_CHECK
    if (skip_fw_version_check()) {
        printf("WARNING: firmware version check is skipped per user request !\n");
        return 1;
    }
#endif

    bd = board_identify(NULL);

    if (p_fw_version_string_info->is_fw_version_no_valid) {
        if (MK_FW_VERSION(p_fw_version_string_info->major, p_fw_version_string_info->minor, p_fw_version_string_info->update) >= bd->required_fw_version) {
            return 1;
        }
    }

    return 0;
}
#endif

#endif

static int
fw_check_header(fw_t* fw, const header_t* h)
{
    fw->type = get_fw_type(h);

    if (!is_fw_type_valid(fw->type)) {
        return -E_TYPE_NOT_MATCH;
    }

    if (!is_checksum_valid(h)) {
        return -E_CHECKSUM_FAILED;
    }

#ifdef RECOVERY_FWVERSION_ARCH
    {
        fw_version_string_info_t fw_version_string_info;

        if (!is_fw_version_string_valid(h)) {
            return -E_VERSION_STRING_CHECK_FAILED;
        }

        parse_fw_version_string(h, &fw_version_string_info);

        if (!is_arch_valid(&fw_version_string_info)) {
            return -E_ARCH_NOT_MATCH;
        }

#ifdef REQUIRED_RECOVERY_FW_VERSION
        if (!pass_fw_version_check(&fw_version_string_info)) {
            return -E_VERSION_CHECK_FAILED;
        }
#endif

    }
#endif

    return 0;
}

static fw_t fw;

fw_t * ubnt_fw_get_info() {
    return &fw;
}

int
ubnt_fw_check(void* base, int size) {
    unsigned long crc;
    header_t* h = (header_t*)base;
    part_t* p;
    part_t* p_old;
    int rc = 0;
    int i;
    signature_t* sig;
    const part_t* p_max = (part_t*)((unsigned char*)base + size);

#ifdef REQUIRED_RECOVERY_FW_VERSION
    ubnt_bdinfo_t * bd = board_identify(NULL);
#endif

    if (size < sizeof(header_t))
        return -1;

    memset(&fw, 0, sizeof(fw_t));

    rc = fw_check_header(&fw, h);

    if (rc) {
        switch (rc) {
            case -E_TYPE_NOT_MATCH:
                printf("ERROR: Firmware Type Error! \n");
                break;
            case -E_CHECKSUM_FAILED:
                printf("ERROR: Firmware Checksum Error! \n");
                break;
#ifdef RECOVERY_FWVERSION_ARCH
            case -E_ARCH_NOT_MATCH:
                printf("ERROR: Firmware Platform Error! \n");
                break;
            case -E_VERSION_STRING_CHECK_FAILED:
                printf("ERROR: Firmware Version String Error!\n");
                break;
#ifdef REQUIRED_RECOVERY_FW_VERSION
            case -E_VERSION_CHECK_FAILED:
                printf("ERROR: Firmware Version Error! (%d.%d.%d or above is needed)\n",
                    GET_FW_MAJOR_VERSION(bd->required_fw_version),
                    GET_FW_MINOR_VERSION(bd->required_fw_version),
                    GET_FW_UPDATE_VERSION(bd->required_fw_version)
                    );
                break;
#endif
#endif
            default:
                printf("ERROR: Unknown Firmware Error! \n");
                break;
        }
        return -2;
    }

    printf("Firmware Version: %s\n", h->version);

    p = (part_t*)((unsigned char*)base + sizeof(header_t));

    i = 0;

    while (strncmp(p->magic, "END.", 4)
#ifdef RSA_IMG_SIGNED
           && strncmp(p->magic, "ENDS", 4)
#endif
    )
    {
#ifdef __DEBUG2
        printf(" Partition (%c%c%c%c): ",
                p->magic[0], p->magic[1], p->magic[2], p->magic[3]);
#endif

    if ((strncmp(p->magic, "PART", 4) == 0) && (i < MAX_PARTITIONS)) {
#ifdef __DEBUG2
        printf("%s [%lu]\n", p->name, ntohl(p->id));
        printf("  Partition size: 0x%X\n", ntohl(p->part_size));
        printf("  Data length: %lu\n", ntohl(p->length));
#endif
        partition_data_t* fwp = &(fw.parts[i]);
        fwp->header = p;
        fwp->data = (unsigned char*)p + sizeof(part_t);
        fwp->signature = (part_crc_t*)(fwp->data + ntohl(p->length));
        crc = ucrc32((unsigned char*)p, ntohl(p->length) + sizeof(part_t));
                    fwp->valid = (htonl(crc) == fwp->signature->crc);
        ++i;
#ifdef __DEBUG2
        printf("  CRC Valid: %s\n", fwp->valid ? "true" : "false");
#endif
        } else {
            if (i >= MAX_PARTITIONS)
                printf("ERROR: too many partitions MAX = %d\n", MAX_PARTITIONS);
            else
                printf("ERROR: did not find expected PART magic\n");
            break;
        }

        p_old = p;
        p = (part_t*)((unsigned char*)p + sizeof(part_t) + ntohl(p->length) + sizeof(part_crc_t));
        if( 0
            || (p < p_old) /* wrap around */
            || (p >= p_max) /* exceed image range */
        ) {
            printf("ERROR: part address error, current address: 0x%08x , previous address: 0x%08x, max address: 0x%08x\n",
                   p, p_old, p_max);
            break;
        }
    }

    fw.part_count = i;

#ifdef __DEBUG2
    printf("Found %d parts\n", fw.part_count);
#endif
    sig = (signature_t*)p;
#ifndef RSA_IMG_SIGNED
    if (strncmp(sig->magic, "END.", 4) != 0) {
#else
    if (strncmp(sig->magic, "ENDS", 4) != 0) {
#endif
#ifdef __DEBUG
        printf("Bad signature!\n");
#endif
        return -3;
    }

#ifndef RSA_IMG_SIGNED
    if (!strncmp(sig->magic, "END.", 4)) {
        crc = ucrc32((unsigned char*)base, (char*)sig - (char*)base);
#ifdef __DEBUG2
        printf("  Signature CRC: 0x%08X\n", sig->crc);
        printf("  Calculated CRC: 0x%08X\n", htonl(crc));
#endif
        if (htonl(crc) != sig->crc) {
#ifdef __DEBUG
            printf("Bad signature CRC!\n");
#endif
            return -4;
        }
    }
#else  //RSA_IMG_SIGNED
    if (!strncmp(sig->magic, "ENDS", 4)) {
        unsigned int img_size  = (char *)sig-(char*)base;

        printf(" RSA Signed Firmware. Verfiying please wait... \n");

        if (ubnt_verify_md(base, img_size, (unsigned char *)sig->rsa_signed_hash, 256)) {
            printf(" Signature authentication failed. Use an official firmware"
                  "  from Ubiquiti Networks Inc. \n");
            return -4;
        }

        if (ubnt_verify_revocation(base, img_size, UAP_MT7621_REVOKED_FW_HASH, sizeof(UAP_MT7621_REVOKED_FW_HASH))) {
            printf(" Release is revoked. UBNT recommends using the latest available firmware for optimal results.\n");
            return -5;
        }

        printf(" Firmware Signature Verfied, Success.\n");
    }
#endif  //CONFIG_RSA_IMG_SIGNED

    return 0;
}


#define I_WANT_TO_WRITE_TO_FLASH  1

/*
 * We have joy, we have fun, we have seasons in the sun...
 */

int
ubnt_update_fw(unsigned char *base, unsigned long size, int bl)
{
    return 0;
}
#if 0
int
ubnt_update_fw(unsigned char *base, unsigned long size, int bl)
{
	int i = 0;
	partition_data_t* p;

	struct part_info *prt;
	mtd_parts_t *part;
	unsigned char pnum;

	int rc;
	unsigned long length;
	unsigned long first_addr, last_addr;

	flash_uinfo_t* flinfo = &flash_info[0];
	unsigned int flash_block_size = flinfo->size / flinfo->sector_count;

        flash_status_display(1);

        for (i = 0; i < fw.part_count; ++i) {

             p = &(fw.parts[i]);

             if (p->valid == 0) {
                 continue;
             }

             if (bl) {
                 if (!strcmp("u-boot", p->header->name)) {
                     printf("Will not overwrite u-boot partition! Skipped.\n");
                     continue;
                 }
             }

             length = (ntohl(p->header->length) + flash_block_size - 1) / flash_block_size;
             length *= flash_block_size;

             printf("Copying partition '%s' to flash memory:\n", p->header->name);

#ifdef I_WANT_TO_WRITE_TO_FLASH
#ifdef QCA_11AC
             if (!is_dual_boot()) {
#endif
                if ((rc=ubnt_flash(p->header->name, p->data, length))) {
                    printf("Error occured while flashing partition '%s'!(%d)\n",
                           p->header->name, rc);
                    flash_status_display(0);
                    return 1;
                }
#ifdef QCA_11AC
            } else {
                if ((rc=ubnt_dual_flash(p->header->name, p->data, length))) {
                    printf("Error occured while flashing partition '%s'!(%d)\n",
                           p->header->name, rc);
                    flash_status_display(0);
                    return 1;
                }
            }
#endif

#else
             printf("FAKE flash_program(%p, %p, 0x%08X)\n", (void*)0, p->data,
                     length);
#endif
        }

	puts("\nFirmware update complete.\n");

        flash_status_display(0);
	return 0;
}
#endif
