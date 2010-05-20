#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct ipfix_identifier {
	uint16_t id;
    uint16_t length;
    uint32_t pen; // enterprise type-id
	char *name;
};

int ipfix_id_rangecheck(int id);
const struct ipfix_identifier* ipfix_id_lookup(uint16_t id, uint32_t pen);
const struct ipfix_identifier* ipfix_name_lookup(const char *name);


#ifdef __cplusplus
}
#endif

#endif
