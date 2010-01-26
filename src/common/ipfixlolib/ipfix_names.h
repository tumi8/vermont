#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct ipfix_identifier {
	char *name;
	uint16_t id;
	//FIXME: length has to be 16 bit long
        uint8_t length;
};

int ipfix_id_rangecheck(int id);
const struct ipfix_identifier * ipfix_id_lookup(int n);
int ipfix_name_lookup(const char *name);


#ifdef __cplusplus
}
#endif

#endif
