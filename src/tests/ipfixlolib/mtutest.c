/*
 * Vermont's MTU Test
 * Copyright (C) 2014 Oliver Gasser
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include <stdio.h>
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#define OBSERVATION_DOMAIN_ID 1
#define TEMPLATE_ID 260
#define COLLECTOR_IP_ADDRESS "127.0.0.1"
// #define COLLECTOR_IP_ADDRESS "87.230.53.17"
// #define COLLECTOR_IP_ADDRESS "8.8.8.8"
#define MTU 0

void define_template(ipfix_exporter *exporter) {
	ipfix_start_template(exporter, TEMPLATE_ID, 1); // field_count == 1
	ipfix_put_template_field(exporter, TEMPLATE_ID, IPFIX_TYPEID_sourceIPv4PrefixLength, 1, 0); // length == 1, enterprise_id == 0
	ipfix_end_template(exporter, TEMPLATE_ID );
}
void put_data(ipfix_exporter *exporter) {
	uint8_t data[256];
	uint8_t i = 0;
	unsigned int j;
	int s;
	for (j=0;j<sizeof(data);j++) {
		data[j] = j;
	}

	ipfix_start_data_set(exporter, htons(TEMPLATE_ID));
	while ((s = ipfix_get_remaining_space(exporter))){
		if (i == 0) {
			ipfix_end_data_set(exporter, 1);
			if (s>IPFIX_OVERHEAD_PER_SET) {
				printf("Starting new data set. Remaining space: %d\n",s);
				ipfix_start_data_set(exporter, htons(TEMPLATE_ID));
			} else {
				printf("Can't start new data set. Remaining Space: %d\n",s);
				break;
			}
		}
		if (ipfix_put_data_field(exporter,&data[i], sizeof(*data))) break;
		i++;
	}
	ipfix_end_data_set(exporter, 1);
	ipfix_send(exporter);
}

void add_udp_collector(ipfix_exporter *exporter) {
	ipfix_aux_config_udp acu = {
		.mtu = 0
	};
	if (ipfix_add_collector(exporter, COLLECTOR_IP_ADDRESS, 4739, UDP, &acu)) {
		fprintf(stderr, "ipfix_add_collector() failed.\n");
		exit(1);
	}
}

void add_dtls_over_udp_collector(ipfix_exporter *exporter) {
	ipfix_aux_config_dtls_over_udp acu = {
		.udp = { .mtu = MTU},
		.dtls = { .peer_fqdn = NULL}
	};
	if (ipfix_add_collector(exporter, COLLECTOR_IP_ADDRESS, 4740, DTLS_OVER_UDP, &acu)) {
		fprintf(stderr, "ipfix_add_collector() failed.\n");
		exit(1);
	}
	while(ipfix_beat(exporter))
		usleep(10000);
}

int main(int argc, char **argv) {
	int i;
	ipfix_exporter *exporter;

	msg_setlevel(MSG_VDEBUG);

	if (ipfix_init_exporter(IPFIX_PROTOCOL, OBSERVATION_DOMAIN_ID, &exporter)) {
		fprintf(stderr, "ipfix_init_exporter() failed.\n");
		exit(1);
	};
	// add_dtls_over_udp_collector(exporter);
	add_udp_collector(exporter);
	define_template(exporter);
	for(i=0;i<3;i++) {
		put_data(exporter);
		sleep(2);
	}
	ipfix_remove_collector(exporter, COLLECTOR_IP_ADDRESS, 4740);
	ipfix_deinit_exporter(&exporter);
	return 0;
}
