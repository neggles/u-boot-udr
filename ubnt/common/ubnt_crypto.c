/*          Copyright Ubiquiti Networks Inc. 2014.
**          All Rights Reserved.
*/


#include <tomcrypt.h>
#include <ubnt_crypto.h>

extern unsigned long __ubnt_app_end;

extern ltc_math_descriptor ltc_mp;
extern const ltc_math_descriptor ltm_desc;

#define RSA_DER_KEY_SIZE 294
#define RSA_PEM_KEY_SIZE 392

#define SHA1_HASH_SIZE 20

/* use this to detect if MAX_RSA_SIZE is big enough in compile time */
struct size_check_struct {
    char check1[MAX_RSA_SIZE - RSA_DER_KEY_SIZE];
    char check2[MAX_RSA_SIZE - RSA_PEM_KEY_SIZE];
};

int ubnt_img_hash(const unsigned char *data, unsigned long datlen,
                  unsigned char *hash, unsigned long *size) {
    hash_state sha1;

    /* setup the hash */
    sha1_init(&sha1);

    sha1_process(&sha1, data, datlen);

    /* get the hash in out[0..15] */

    sha1_done(&sha1, hash);

    *size = SHA1_HASH_SIZE;

    return CRYPT_OK;
}

static const char key[] = "\x81\x93\xe0\xc4";
static const size_t key_len = 4;

static void
descramble(const unsigned char* in, unsigned char* out, unsigned int datalen)
{
    unsigned int i;

    for (i = 0; i < datalen; ++i) {
        out[i] = in[i] ^ key[ i % key_len ];
    }
}

int
ubnt_verify_revocation(const unsigned char *data, unsigned long datlen,
                       const unsigned char *revocation_list_in, unsigned long revocation_list_len) {
    int rc = 0;
    unsigned char hash[SHA1_HASH_SIZE];
    unsigned long hashlen;
    unsigned long i;
    unsigned char *revocation_list;

    if(revocation_list_len == 0)
        return 0;

    revocation_list = XMALLOC(revocation_list_len);

    descramble(revocation_list_in, revocation_list, revocation_list_len);

    if ((rc = ubnt_img_hash(data, datlen, hash, &hashlen))) {
        printf("Img hash failed, rc=%d \n", rc);
        rc = -1;
        goto cleanup;
    }

    for (i = 0; i < revocation_list_len; i += SHA1_HASH_SIZE) {
        if (memcmp(hash, &revocation_list[i], SHA1_HASH_SIZE) == 0) {
            rc = -1;
            goto cleanup;
        }
    }

cleanup:
    XFREE(revocation_list);
    return rc;
}

int
ubnt_verify_md(const unsigned char *data, unsigned long datlen,
               const unsigned char *sig, unsigned long siglen) {
    unsigned char *key_material = (unsigned char *)(&__ubnt_app_end);
    int rc=0, stat;
    unsigned char hash[20];
    unsigned long hashlen;
    rsa_key key;
    unsigned char *der_key;
    unsigned long der_key_len = 0;

    ltc_mp = ltm_desc;

    der_key = XCALLOC(1, MAX_RSA_SIZE);
    if (der_key == NULL) {
        printf("Allocate memory failed.\n");
        return -1;
    }

    /* Descramble the key that was scrambled during build */
#ifdef CONFIG_USE_PEM_PUBLIC_KEY
    descramble(key_material, der_key, RSA_PEM_KEY_SIZE);
    der_key_len = MAX_RSA_SIZE;
    /* note: use the same buffer for both input and output */
    rc = base64_decode(der_key, RSA_PEM_KEY_SIZE, der_key, &der_key_len);

    if (rc != CRYPT_OK) {
        XFREE(der_key);
        printf("Key decode failed, rc=%d\n", rc);
        return -1;
    }
#else
    descramble(key_material, der_key, RSA_DER_KEY_SIZE);
    der_key_len = RSA_DER_KEY_SIZE;
#endif

    rc = rsa_import(der_key, der_key_len, &key);
    XFREE(der_key);

    if (rc != CRYPT_OK) {
        printf("Key import failed, rc=%d\n", rc);
        return -1;
    }

    if ((rc = ubnt_img_hash(data, datlen, hash, &hashlen))) {
        printf("Img hash failed, rc=%d \n", rc);
        return -1;
    }

    if (register_hash(&sha1_desc) == -1) {
        printf("Hash registration failed \n");
        return -1;
    }

    if ((rc = rsa_verify_hash_ex(sig, siglen, hash, hashlen,
                    LTC_LTC_PKCS_1_V1_5, 0, 0, &stat, &key)) != CRYPT_OK) {
#ifdef DEBUG
        printf("\n Message Digest verification failed, rc=%d \n", rc);
#endif
        return -1;
    }

    return stat?0:-1;
}

void
print_key(char *key, int size) {
    int i=0;

    printf ("\nKey: %x\n", (int)key);

    for (i=0; i < size; i++)
         printf("%02x ", key[i]);

    printf("\n");

}

