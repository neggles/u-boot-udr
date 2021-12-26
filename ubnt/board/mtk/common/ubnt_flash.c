#include <ubnt_config.h>
#include <exports.h>
#include <config.h>
#include <ubnt_flash.h>
#include <ubnt_mtd.h>
#include <ubnt_types.h>

extern flash_uinfo_t flash_info[];

/* APIs for flash read/write/erase */

int ubnt_flash_read(unsigned char *name, unsigned char *addr) {
    unsigned int sa=0, sz=0;

    get_mtd_params(name, &sa, &sz, NULL, NULL);

    printf("%s: addr=%x, sa=%x, sz=%d \n", __func__, addr, sa, sz);
    memcpy(addr, sa, sz);

    return 0;
}

    /* Update the flash at the given offset for the given size. If size is
    ** is zero, don't do anything.
    */
int ubnt_flash_update(unsigned char *name, unsigned char *addr,
                      size_t data_sz) {
    int rc=0, sa=0, sz=0, ss=0, es=0;
    flash_uinfo_t* flinfo = &flash_info[0];

    get_mtd_params(name, &sa, &sz, &ss, &es);

    if (data_sz && sz) {
        flash_erase(flinfo, ss, es);
        write_buff(flinfo, addr, sa, data_sz);
    }

    return  rc;
}

int ubnt_flash_erase(unsigned char *name) {
    int rc=0;
    flash_uinfo_t* flinfo = &flash_info[0];
    int sa, sz, ss, es;

    get_mtd_params(name, &sa, &sz, &ss, &es);

    if (sz)
        flash_erase(flinfo, ss, es);

    return rc;
}
