/* vim: set sts=4 sw=4 cindent nowrap: This modeline was added by Daniel Mentz */
#ifdef SUPPORT_DTLS

#include "common/openssl/OpenSSL.h"
#include "common/Mutex.h"
#include "common/msg.h"
#include <openssl/ssl.h>
#include <openssl/x509v3.h>
#include <openssl/err.h>
#include <sstream>
#include <cstring>

namespace { /* unnamed namespace */
    Mutex m;
    bool initialized = false; /* Determines wether OpenSSL is initialized already */
    int check_x509_cert(X509 *peer, int (*cb)(void *context, const char *dnsname), void *context);
    int ex_data_idx_vpcd = 0; /* Must be set by to return value from SSL_get_ex_new_index() before use */
#define SSL_ERR(c) {c,#c}

    struct sslerror {
	int code;
	const char *str;
    } sslerrors[] = {
	SSL_ERR(SSL_ERROR_NONE),
	SSL_ERR(SSL_ERROR_ZERO_RETURN),
	SSL_ERR(SSL_ERROR_WANT_READ),
	SSL_ERR(SSL_ERROR_WANT_WRITE),
	SSL_ERR(SSL_ERROR_WANT_ACCEPT),
	SSL_ERR(SSL_ERROR_WANT_CONNECT),
	SSL_ERR(SSL_ERROR_WANT_X509_LOOKUP),
	SSL_ERR(SSL_ERROR_SYSCALL),
	SSL_ERR(SSL_ERROR_SSL),
    };
}

void ensure_openssl_init(void) {
    m.lock();
    if ( ! initialized) { /* skip this if we already initialized OpenSSL */
	initialized = true;
	SSL_load_error_strings(); /* readable error messages */
	SSL_library_init(); /* initialize library */
	ex_data_idx_vpcd = SSL_get_ex_new_index(0, NULL, NULL, NULL, NULL);

#if 0
	if (SSL_COMP_add_compression_method(0, COMP_zlib())) {
	    msg(LOG_ERR, "OpenSSL: SSL_COMP_add_compression_method() failed.");
	    msg_openssl_errors();
	};
#endif
	DPRINTF_INFO("Initialized OpenSSL");
    }
    m.unlock();
}

/* Get errors from OpenSSL error queue and output them using msg() */
void msg_openssl_errors(void) {
    char errbuf[512];
    char buf[4096];
    unsigned long e;
    const char *file, *data;
    int line, flags;

    while ((e = ERR_get_error_line_data(&file,&line,&data,&flags))) {
	ERR_error_string_n(e,errbuf,sizeof errbuf);
	snprintf(buf, sizeof buf, "%s:%s:%d:%s", errbuf,
                        file, line, (flags & ERR_TXT_STRING) ? data : "");
	msg(LOG_ERR, "OpenSSL: %s",buf);
    }
}

void msg_openssl_return_code(int level, const char *fn, int ret, int error) {
    std::ostringstream oss;
    oss << fn << " returned: " << ret;
    if (ret<=0) {
	oss << ", error: ";
	unsigned int i;
	int found=0;
	for(i=0;i < sizeof(sslerrors) / sizeof(struct sslerror);i++) {
	    if (sslerrors[i].code==error) {
		oss << sslerrors[i].str;
		found=1;
		break;
	    }
	}
	if (!found) {
	    oss << error;
	}
	if (error == SSL_ERROR_SYSCALL && ret==-1)
	    oss << ", errno: " << strerror(errno);
    }
    msg(level, "%s", oss.str().c_str());
    msg_openssl_errors();
}

#if 0
void msg_openssl_return_code(const char *fn, int ret, int error) {
	ostringstream oss;
	
	oss << "<receivedPackets>" << statReceivedPackets << "</receivedPackets>" << endl;	

	return oss.str();
    if (ret > 0) {
	DPRINTF_INFO("%s returned: %d",fn,ret);
    } else {
	char buf[16];
	char *s = 0;
	unsigned int i;
	for(i=0;i < sizeof(sslerrors) / sizeof(struct sslerror);i++) {
	    if (sslerrors[i].code==error) {
		s = sslerrors[i].str;
		break;
	    }
	}
	if (!s) {
	    snprintf(buf,sizeof(buf),"%d",error);
	    s = buf;
	}
	DPRINTF_INFO("%s returned: %d, error: %s",

    DPRINTF_INFO("SSL_read() returned: %d, error: %d, strerror: %s",ret,error,strerror(errno));
}
#endif


const char *get_ssl_error_string(int error) {
    unsigned int i;
    static char unknown[] = "Unknown error code: %d";
    static char s[sizeof(unknown) + 20];
    for(i=0;i < sizeof(sslerrors) / sizeof(struct sslerror);i++) {
        if (sslerrors[i].code==error) {
            return sslerrors[i].str;
        }
    }
    snprintf(s, sizeof(s), unknown, error); /* Not thread safe. */
    return s;
}

/* returns 1 on success, 0 on error */
int verify_ssl_peer(SSL *ssl, int (*cb)(void *context, const char *dnsname), void *context) {
    long verify_result;

    verify_result = SSL_get_verify_result(ssl);
    DPRINTF_INFO("SSL_get_verify_result() returned: %s",X509_verify_cert_error_string(verify_result));
    if(verify_result!=X509_V_OK) {
	msg(LOG_ERR,"Certificate doesn't verify: %s", X509_verify_cert_error_string(verify_result));
	return 0;
    }

    X509 *peer = SSL_get_peer_certificate(ssl);
    if (! peer) {
	msg(LOG_ERR,"No peer certificate");
	return 0;
    }
    int ret = check_x509_cert(peer, cb, context);
    X509_free(peer);
    return ret;
}

int verify_peer_cert_callback(int preverify_ok, X509_STORE_CTX *ctx) {
    char buf[512];
    SSL *ssl;
    struct verify_peer_cb_data *cb_data;
    X509 *cert = X509_STORE_CTX_get_current_cert(ctx);
    int depth = X509_STORE_CTX_get_error_depth(ctx);
    int err = X509_STORE_CTX_get_error(ctx);
    if(!preverify_ok) {
	msg(LOG_ERR,"Error with certificate at depth: %i",depth);
	X509_NAME_oneline(X509_get_issuer_name(cert),buf,sizeof(buf));
	msg(LOG_ERR," issuer = %s",buf);
	X509_NAME_oneline(X509_get_subject_name(cert),buf,sizeof(buf));
	msg(LOG_ERR," subject = %s",buf);
	msg(LOG_ERR," err %i:%s", err, X509_verify_cert_error_string(err));
    }
    if (depth == 0) {
	ssl = (SSL*) X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	cb_data = (verify_peer_cb_data *) SSL_get_ex_data(ssl, get_openssl_ex_data_idx_vpcd());
	if (cb_data)
	    return check_x509_cert(cert,cb_data->cb,cb_data->context);
    }
    return preverify_ok;
}

int get_openssl_ex_data_idx_vpcd(void) {
    return ex_data_idx_vpcd; /* Must be set by to return value from SSL_get_ex_new_index() before use */
}

namespace {

/* returns 1 on success, 0 on error */
int check_x509_cert(X509 *peer, int (*cb)(void *context, const char *dnsname), void *context) {
    char buf[512];
#if DEBUG
    X509_NAME_oneline(X509_get_subject_name(peer),buf,sizeof buf);
    DPRINTF_INFO("peer certificate subject: %s",buf);
#endif
    const STACK_OF(GENERAL_NAME) * gens;
    const GENERAL_NAME *gn;
    int num;
    size_t len;
    const char *dnsname;

    gens = (STACK_OF(GENERAL_NAME) *) X509_get_ext_d2i(peer, NID_subject_alt_name, 0, 0);
    num = sk_GENERAL_NAME_num(gens);
    num = sk_num(((_STACK*) (1 ? (gens) : (struct stack_st_GENERAL_NAME*)0)));

    for (int i = 0; i < num; ++i) {
	gn = sk_GENERAL_NAME_value(gens, i);
	if (gn->type != GEN_DNS)
	    continue;
	if (ASN1_STRING_type(gn->d.ia5) != V_ASN1_IA5STRING) {
	    msg(LOG_ERR, "malformed X509 cert: Type of ASN.1 string not IA5");
	    return 0;
	}

	dnsname = (char *) ASN1_STRING_get0_data(gn->d.ia5);
	len = ASN1_STRING_length(gn->d.ia5);

	while(len>0 && dnsname[len-1] == 0) --len;

	if (len != strlen(dnsname)) {
	    msg(LOG_ERR, "malformed X509 cert");
	    return 0;
	}
	DPRINTF_INFO("Subject Alternative Name: DNS:%s",dnsname);
	if ( (*cb)(context, dnsname) ) {
	    DPRINTF_INFO("Subject Alternative Name matched one of the "
		    "permitted FQDNs");
	    return 1;
	}

    }
    if ((len = X509_NAME_get_text_by_NID
	    (X509_get_subject_name(peer),
	     NID_commonName, buf, sizeof buf)) <=0 ) {
	DPRINTF_INFO("CN not part of certificate");
    } else {
	if (len != strlen(buf)) {
	    msg(LOG_ERR,"malformed X509 cert: CN invalid");
		    return 0;
	}
	DPRINTF_INFO("most specific (1st) Common Name: %s",buf);
	if ( (*cb)(context, buf) ) {
	    DPRINTF_INFO("Common Name (CN) matched one of the "
		    "permitted FQDNs");
	    return 1;
	}
    }
    msg(LOG_ERR,"Neither any of the Subject Alternative Names nor the Common Name "
	    "matched one of the permitted FQDNs");
    return 0;
}

} /* unnamed namespace */

#endif /* SUPPORT_DTLS */

