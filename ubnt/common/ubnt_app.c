
/*
**          Copyright Ubiquiti Networks Inc. 2014
**          All Rights Reserved.
*/

#include <stdarg.h>
#include <exports.h>
#include <types.h>
#include <ubnt_types.h>
#include <ubnt_config.h>

/* UBNT APP Header */

typedef struct {
     char ubnt_magic[4];
     unsigned char version;
} ubnt_app_hdr_t;

ubnt_app_hdr_t ubnt_app_hdr __attribute__((section(".ubnt_hdr")))
    = {
    .ubnt_magic = {0x87, 0x56, 0x45, 0x23},
    .version = 0x01
};

ubnt_app_persistent_data_t ubnt_persistent_data __attribute__((section(".ubnt_data")));

unsigned long
ubnt_app(int argc, char *argv[]) __attribute__((section(".ubnt_start")));
    /* Macro to compare and call the command functions */
#define UBNT_API_CALL(x)  { \
                        extern ubnt_exec_t ubnt_##x(int, char **);\
                        if (strncmp(#x, argv[1], strlen(#x)) == 0) { \
                            rc = ubnt_##x(--argc, &argv[1]); \
                        } \
                     }

/* This function is called from the u-boot code to issue ubnt commands.
   NOTE: This function assumes all the values passed are correct. Please
   validate the values before its passed to this function.
*/
unsigned long
ubnt_app(int argc, char *argv[])
{
    ubnt_exec_t rc;

    //app_startup(argv);
    char *app_init_done = getenv("appinitdone");

    if (argc < 1) {
        printf(" Invalid command, argc=%d \n");
        return 1;
    }

    /* Initialize the BSS segements only once during ubnt app initialization.
       Otherwise peristent data in .bss will be lost between commands.
    */
    if (strncmp("uappinit", argv[1], 11) == 0) {
        app_startup(argv);
        ucheckboard();
        setenv("appinitdone", "true");
        UBNT_API_CALL(uappinit);
        return rc.ret;
    }


    /* Beyond this, commands can be called only after the app is initialized
       first. Otherwise the state of BSS is indeterministic and might erase
       the wrong part of flash if flash write command is issued.
    */

    if (!app_init_done) {
        printf(" Initialize the APP first \n");
        return 0;
    }

    UBNT_API_CALL(uappcontinue);
    UBNT_API_CALL(uclearenv);
    UBNT_API_CALL(uclearcfg);
    UBNT_API_CALL(usetled);
    UBNT_API_CALL(ucheckboard);
#ifdef UNIFI_DETECT_RESET_BTN
    UBNT_API_CALL(ureset_button);
#endif
    UBNT_API_CALL(uwrite);
    UBNT_API_CALL(uprintenv);
    UBNT_API_CALL(usetenv);
    UBNT_API_CALL(usaveenv);
#ifdef QCA_11AC
    UBNT_API_CALL(ubntboot);
#endif
#ifdef DEBUG /* WARNING: ***DO NOT*** enable this command in release build */
    UBNT_API_CALL(usetprotect);
#endif
    UBNT_API_CALL(uramload);
    return rc.ret;
}
