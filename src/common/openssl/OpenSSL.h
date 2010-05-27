/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */

#ifndef OPENSSLINIT_H
#define OPENSSLINIT_H

#ifdef SUPPORT_DTLS

#ifdef __cplusplus
extern "C" {
#endif

#include <openssl/ssl.h>

struct verify_peer_cb_data {
    int (*cb)(void *context, const char *dnsname);
    void *context;
};

void ensure_openssl_init(void);
void msg_openssl_errors(void);
void msg_openssl_return_code(int level, const char *fn, int ret, int error);
int verify_ssl_peer(SSL *ssl, int (*cb)(void *context, const char *dnsname), void *context);
int verify_peer_cert_callback(int preverify_ok, X509_STORE_CTX *ctx);
int get_openssl_ex_data_idx_vpcd(void); /* vpcd = verify_peer_cb_data */
const char *get_ssl_error_string(int ret);

#ifdef __cplusplus
}
#endif

#endif /* SUPPORT_DTLS */

#endif
