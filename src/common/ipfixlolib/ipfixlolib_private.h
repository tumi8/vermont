#ifndef IPFIXLOLIB_PRIVATE_H
#define IPFIXLOLIB_PRIVATE_H

#include "ipfixlolib.h"
#include "ipfixlolib_config.h"

int enable_pmtu_discovery(int s);
int update_collector_mtu(ipfix_exporter *exporter, ipfix_receiving_collector *col);
void ipfix_update_header(ipfix_exporter *p_exporter, ipfix_receiving_collector *collector, ipfix_sendbuffer *sendbuf);
void set_mtu_config(ipfix_receiving_collector *col, ipfix_aux_config_udp *aux_config_udp);
void update_exporter_max_message_size(ipfix_exporter *exporter);

#endif
