#ifndef IPFIXLOLIB_DTLS_PRIVATE_H
#define IPFIXLOLIB_DTLS_PRIVATE_H

#include "ipfixlolib.h"
#include "ipfixlolib_dtls.h"

int dtls_manage_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col);
void deinit_openssl_ctx(ipfix_exporter_certificate *certificate);
int setup_dtls_connection(ipfix_exporter *exporter, ipfix_receiving_collector *col, ipfix_dtls_connection *con);
void dtls_shutdown_and_cleanup(ipfix_dtls_connection *con);
int dtls_send_templates(ipfix_exporter *exporter, ipfix_receiving_collector *col);
int dtls_send(ipfix_exporter *exporter, ipfix_receiving_collector *col, const struct iovec *iov, int iovcnt);
int add_collector_dtls(ipfix_exporter *exporter, ipfix_receiving_collector *col, void *aux_config);
int ipfix_dtls_advance_connections(ipfix_exporter *exporter);

#endif
