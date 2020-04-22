#ifndef IPFIXLOLIB_DTLS_H
#define IPFIXLOLIB_DTLS_H

#include <time.h>
#include <openssl/ssl.h>

#include "ipfixlolib_config.h"

#ifdef __cplusplus
extern "C" {
#endif

#define IPFIX_DTLS_MAX_RECORD_LENGTH 16384

/* DTLS record header:
 *   ContentType: 1 byte
 *   ProtocolVersion: 2 bytes
 *   epoch: 2 bytes
 *   seqno: 6 bytes
 *   length: 2 bytes
 *   (assuming GenericBlockCipher, AES_256_CBC and SHA256)
 *   IV: 16 bytes
 *   MAC: 32 bytes
 *   padding: 15 bytes (worst case)
 *   padding_length: 1 byte */
#define IPFIX_DTLS_OVERHEAD_ESTIMATE 77

typedef struct {
	int socket;
	// uint16_t mtu;
	SSL *ssl;
	time_t last_reconnect_attempt_time;
} ipfix_dtls_connection;

typedef struct {
	/* Time in seconds after which a DTLS connection
	 * will be replaced by a new one. */
	unsigned dtls_max_connection_lifetime;
	unsigned dtls_connect_timeout;
	ipfix_dtls_connection dtls_main;
	ipfix_dtls_connection dtls_replacement;
	time_t connect_time; /* point in time when the connection setup
				succeeded. We need this to calculate the
				age of a connection. If DTLS is used,
				a connection rollover is performed when
				a connection reaches a certain age.*/
	const char *peer_fqdn;
} ipfix_collector_dtls_connection;

typedef struct {
    const char *peer_fqdn; /*!< The Fully Qualified Domain Name (FQDN) of the
			     peer. If set, the peer i.e. the Collector
			     <em>must</em> present a certificate of which
			     either the subject's Common Name (CN) or one of
			     the subject alternative names matches the FQDN.
			     There is no support for wildcard matching. For the
			     certificate verification to work, the user must
			     also call <tt>ipfix_set_ca_locations()</tt> in
			     advance to specify the locations of the root CA
			     certificates.

			     If set to NULL, anonymous cipher suites will be
			     added to the list of permissible cipher suites.
			     The identity of the peer will not be verified
			     then.*/
} ipfix_aux_config_dtls;

typedef struct {
    ipfix_aux_config_dtls dtls; /*!< DTLS specific configuration */
    ipfix_aux_config_udp udp; /*!< UDP specific configuration */
    unsigned max_connection_lifetime; /*!< Time in seconds after which the DTLS
				 connection is replaced by a new one.
				 This mechanism aims to overcome the
				 dead peer problem.*/
} ipfix_aux_config_dtls_over_udp;

typedef struct {
    ipfix_aux_config_dtls dtls; /*!< DTLS specific configuration */
} ipfix_aux_config_dtls_over_sctp;

typedef struct {
	SSL_CTX *ssl_ctx;
	const char *certificate_chain_file;
	const char *private_key_file;
	const char *ca_file;
	const char *ca_path;
} ipfix_exporter_certificate;

void ipfix_init_dtls_certificate(ipfix_exporter_certificate *certificate);
void ipfix_clear_dtls_certificate(ipfix_exporter_certificate *certificate);
int ipfix_set_dtls_certificate(ipfix_exporter_certificate *certificate,
				    const char *certificate_chain_file, const char *private_key_file);
int ipfix_set_ca_locations(ipfix_exporter_certificate *certificate,
				const char *ca_file, const char *ca_path);

#ifdef __cplusplus
}
#endif

#endif
