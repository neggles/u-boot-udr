
#include <common.h>
#include <exports.h>
#include <config.h>
#include <ubnt_config.h>
#include <ubnt_types.h>
#define U_BOOT_ENV  "u-boot-env"

#if 0
#define NULL 0

typedef struct beacon_packet {
    int           ret; /* Note: Overlaps with ret in the union struct below */
    unsigned char *ptr;
    int           len;
} beacon_pkt_t;

typedef union ubnt_exec {
    int ret;
    beacon_pkt_t beacon_pkt;
} ubnt_exec_t;
#endif

//KMLUOH: FIXME
#define CFG_ENV_SIZE    65536
typedef struct __attribute__((__packed__)) env {
    unsigned long crc;
    char          data[CFG_ENV_SIZE];
} env_t;

/* u-boot malloc is very primitive. Don't depend on it for big sizes.
  Instead let the compiler deal with it. BTW, This does not consume memory
  in the flash as the memory is allocated during run time during the program
  loading. */
static char new_tmp[CFG_ENV_SIZE];

static env_t ubnt_env;

#define STR(s) #s
#define UBNT_STR(s) STR(s)

static char default_env[] = {
#if defined(CONFIG_BOOTARGS)
    "bootargs="     CONFIG_BOOTARGS                 "\0"
#endif
#if defined(CONFIG_BOOTCOMMAND)
    "bootcmd="      CONFIG_UBNTBOOTCOMMAND          "\0"
#endif
#if defined(CONFIG_BOOTDELAY) && (CONFIG_BOOTDELAY >= 0)
    "bootdelay="    UBNT_STR(CONFIG_BOOTDELAY)      "\0"
#endif
#ifdef	CONFIG_IPADDR
    "ipaddr="       UBNT_STR(CONFIG_IPADDR)         "\0"
#endif
#ifdef	CONFIG_SERVERIP
    "serverip="     UBNT_STR(CONFIG_SERVERIP)       "\0"
#endif
#ifdef CONFIG_UBNTAPPINIT
    "ubntappinit="  CONFIG_UBNTAPPINIT              "\0"
#endif
#ifdef DEBUG
#ifdef CONFIG_BREV
    "usetbrev="     CONFIG_BREV                     "\0"
#endif
#ifdef DEBUG
#ifdef CONFIG_PROTECT
    "usetprotect="  CONFIG_PROTECT                  "\0"
#endif
#endif
    "usetbid="      CONFIG_BID                      "\0"
#ifdef CONFIG_MAC
    "usetmac="      CONFIG_MAC                      "\0"
#endif
#ifdef CONFIG_RD
    "usetrd="       CONFIG_RD                       "\0"
#endif
#endif /* DEBUG */
#ifdef UBNT_APP_START_OFFSET
    "ubntaddr="     UBNT_STR(UBNT_APP_START_OFFSET) "\0"
#endif
#ifdef MTDPARTS_INITRAMFS_SIGNED_16
    "mtdparts="     MTDPARTS_INITRAMFS_SIGNED_16    "\0"
#endif
#ifdef CONFIG_EXTRA_ENV_SETTINGS
    CONFIG_EXTRA_ENV_SETTINGS
#endif
    "\0"
};


/* Reset uboot env if any of the matching string is found */

static void
ubnt_reset_ubootenv(char *str) {
    char *env = ubnt_env.data;
    char *match;
    int matched = 0;

    while (env && *env != '\0') {
        match = (char *)strtok(env, "=");
        while ((match=(char *)strtok(NULL, " "))) {
            if (!strcmp(match, str)) {
                matched = 1;
                break;
            }
        }
        env += strlen(env)+1;
    }

    if (matched) {
        printf("uboot env fix. Clearing u-boot env and resetting the board.. \n");
        ubnt_uclearenv();
        //FIXME: ath_reset();
    }

}

/* Read the environment from flash or set to default if its invalid */
void ubnt_env_init() {
#if 0
    char *ubntappinitp = NULL;
    unsigned int *flash_env_addr = (unsigned int *)get_mtd_saddress(U_BOOT_ENV);

    ubnt_env.crc = *flash_env_addr;

    /* Copy from flash to heap */
    memcpy(ubnt_env.data, (char *)(flash_env_addr+1), CFG_ENV_SIZE);

    /* If the CRC of flash env is invalid fill it with default */

    if (ubnt_env.crc != ucrc32(ubnt_env.data, CFG_ENV_SIZE-4)) {

        printf("u-boot-env is invalid, overiding with default.\n");
        memcpy(ubnt_env.data, default_env, sizeof(default_env));
        ubnt_env.crc = ucrc32(ubnt_env.data, CFG_ENV_SIZE-4);

        /* Write to flash */
        ubnt_flash(U_BOOT_ENV, (char *)&ubnt_env, CFG_ENV_SIZE);
    } else {
        ubntappinitp = NULL;
        ubntappinitp = getenv("ubntappinit");
        /* take care of upgrade from non-ubntapp u-boot */
        if ((ubntappinitp == NULL) || strlen(ubntappinitp) == 0) {
            printf("ubntappinit not found in uboot-env, overiding with default.\n");
            memcpy(ubnt_env.data, default_env, sizeof(default_env));
            ubnt_env.crc = ucrc32(ubnt_env.data, CFG_ENV_SIZE-4);

            /* Write to flash */
            ubnt_flash(U_BOOT_ENV, (char *)&ubnt_env, CFG_ENV_SIZE);
        }
    }

    ubnt_reset_ubootenv("console=0");
#endif
}

ubnt_exec_t
ubnt_uprintenv(int argc, char *argv[]) {
    ubnt_exec_t res;
    char *env = ubnt_env.data;

    res.ret = 0;

    /* If consecutive \0s then its end of the env */
    while (env && *env != '\0') {
        printf("%s\n", env);
        env += strlen(env)+1;
    }

    return res;
}

ubnt_exec_t
ubnt_usetenv(int argc, char *argv[]) {
    ubnt_exec_t res;
    char *new_env=new_tmp, *env=ubnt_env.data;
    char *token;
    int matched=0, i;

    res.ret = 0;


    /* If invalid command passed, return silently */
    if (argc < 2) {
        return res;
    }

    env = ubnt_env.data;

    token = (char *) strtok(env, "=");

    /* Get the token out */
    while (token) {

        /* If there is a match do what is requested */
        if (!strncmp(token, argv[1], strlen(argv[1]))) {
            matched = 1;

            /* Move the env so that it will point to next entry
             for memcpy to move in matched section below */
            env += strlen(token)+1;
            /* User wants to clear the setting */
            if (argc == 2) break;

            memcpy(new_env, token, strlen(token));
            new_env += strlen(token);
            strcpy(new_env, "=");
            new_env++;

            for (i=2;  i < argc; i++) {

                /* Copy the new env from user */
                 memcpy(new_env, argv[i], strlen(argv[i]));
                 new_env += strlen(argv[i]);

                /* Add space after first sub string */
                strcpy(new_env++, " ");

            }
            strcpy(new_env++, "\0");
            break;
       }

       memcpy(new_env, token, strlen(token));
        /* Skip past \0. Otherwise strtok will return NULL */
       env += strlen(token)+1;
       new_env +=  strlen(token);
       strcpy(new_env++, "=");

       token = (char *) strtok(NULL, "\0");
       memcpy(new_env, token, strlen(token) + 1);
       env += strlen(token) + 1;
       new_env +=  strlen(token) + 1;

        /* get the next token */
       token = (char *) strtok(env, "=");
    }

    if (matched && token) {
        /* Move the rest of the env. Discard the previous matched token body.*/
        token = (char *) strtok(NULL, "\0");
        env += strlen(token) + 1;
        memcpy(new_env, env, CFG_ENV_SIZE - (env - ubnt_env.data));
    }

    /* A new entry and if more than a token is present add it to the end */
    if (!matched && argc >=3 ) {

        memcpy(new_env, argv[1], strlen(argv[1]));
        new_env += strlen(argv[1]);
        strcpy(new_env++, "=");

        /* Add to the end of the list */
        for (i=2;  i < argc; i++) {

           /* Copy the new env from user */
           memcpy(new_env, argv[i], strlen(argv[i]));
           new_env += strlen(argv[i]);

           /* Add space after first sub string */
           strcpy(new_env++, " ");
        }
        strcpy(new_env++, "\0");
   }

    /* Copy back to the original env storage */
    memcpy(ubnt_env.data, new_tmp, CFG_ENV_SIZE);

    return res;
}

ubnt_exec_t
ubnt_usaveenv(int argc, char *argv[]) {
    ubnt_exec_t res;
    res.ret = 0;

    ubnt_env.crc = ucrc32(ubnt_env.data, CFG_ENV_SIZE-4);

    /* Write to flash */
    ubnt_flash(U_BOOT_ENV, (char *)&ubnt_env, CFG_ENV_SIZE);

    return res;
}
