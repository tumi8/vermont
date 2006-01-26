#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPFIX_ENTERPRISE_FLAG (1 << 15)

struct ipfix_identifier {
	char *name;
	uint16_t id;
        uint8_t length;
};

int ipfix_id_rangecheck(int id);
const struct ipfix_identifier * ipfix_id_lookup(int n);
int ipfix_name_lookup(char *name);


#ifdef __cplusplus
}
#endif

#endif
