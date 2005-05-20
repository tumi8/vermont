/*
 this is vermont.
 released under GPL v2

 (C) by Ronny T. Lampert
 */

#define HOOK_SOURCE_ID 4913
/* just for abs */
#include <stdlib.h>

#include "aggregator.h"
#include "rcvIpfix.h"

#include "sampler/packet_hook.h"
#include "sampler_hook_entry.h"

#include "msg.h"

#ifdef __cplusplus
extern "C" {
#endif

static FieldInfo ip_traffic_fi[] = {
	/*
	 { { ID, len, enterprise}, offset} for:
	 proto
	 srcip
	 dstip
	 srcport
	 dstport
	 */
	{ {4, 1, 0}, 9},
	{ {8, 4, 0}, 12},
	{ {12, 4, 0}, 16},
	{ {7, 2, 0}, 0},
	{ {11, 2, 0}, 2}
};

static TemplateInfo ip_traffic_template = {
	5,
	ip_traffic_fi,
	NULL
};

/*
 this is the function called by sampler::HookingFilter
 the entrypoint into the concentrator
 this is a mess and not thread safe
 */

void sampler_hook_entry(void *ctx, void *data)
{
	int transport_offset;
	struct packet_hook *ph=(struct packet_hook *)data;
        void *aggregator=ctx;
	FieldData *fdata=(FieldData *)ph->ip_header;
	
	/*
	 because of IP options we need to re-calculate the offsets to srcport and dstport every time
	 now we do need some serious pointer arithmetic:
	 - calculate the offset of transport header to ip header
	 - use this offset and add to src/dst_port offset
	 */
	transport_offset=abs(ph->transport_header - ph->ip_header);
	//DPRINTF("hook_entry: length is %d\n", ph->length);

	ip_traffic_template.fieldInfo[3].offset += transport_offset;
	ip_traffic_template.fieldInfo[4].offset += transport_offset;

	//aggregateDataRecord(aggregator, HOOK_SOURCE_ID, &ip_traffic_template, ph->length, fdata);

	/* reset ip_traffic_fi, offsets for srcport/dstport to starting values */
	ip_traffic_template.fieldInfo[3].offset = 0;
	ip_traffic_template.fieldInfo[4].offset = 2;

}

#ifdef __cplusplus
}
#endif
