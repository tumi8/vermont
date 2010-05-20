#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct ipfix_identifier {
	uint16_t id;
    uint16_t length;
	char *name;
};

int ipfix_id_rangecheck(int id);
const struct ipfix_identifier * ipfix_id_lookup(int n);
int ipfix_name_lookup(const char *name);


#ifdef __cplusplus
}
#endif

#endif
