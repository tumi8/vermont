#ifndef PACKET_HOOK_H
#define PACKET_HOOK_H

struct packet_hook {

	void *data;
	void *ip_header;
	void *transport_header;

	const struct timeval *timestamp;

	int length;
};



#endif
