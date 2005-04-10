#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* FIXME:
 dirty defines for the moment; full naming defines should follow
 */
#define IPFIX_ID_MIN 1
#define IPFIX_ID_MAX 140
#define PSAMP_ID_MIN 1024
#define PSAMP_ID_MAX 1037

#define PROTO_ICMP 1
#define PROTO_IGMP 2
#define PROTO_TCP 6
#define PROTO_UDP 17
/* ??? */
#define PROTO_RAW 255

#define IPFIX_ENTERPRISE_FLAG (1 << 15)

struct ipfix_identifier {
	char *name;
	uint16_t id;
        uint8_t length;
};

const struct ipfix_identifier * ipfix_id_lookup(int n);
int ipfix_name_lookup(char *name);


#ifdef __cplusplus
}
#endif

#endif
