// vim: ts=4:sw=4:expandtab

/*          Copyright Ubiquiti Networks Inc. 2016.
**          All Rights Reserved.
*/
#ifndef UBNT_CRYPTO_H
#define UBNT_CRYPTO_H


int
ubnt_verify_revocation(const unsigned char *data, unsigned long datlen,
                       const unsigned char *revocation_list_in, unsigned long revocation_list_len);

int
ubnt_verify_md(const unsigned char *data, unsigned long datlen,
               const unsigned char *sig, unsigned long siglen);

#endif /* UBNT_CRYPTO_H */
