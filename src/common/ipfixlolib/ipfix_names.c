/*
 * Enterprise specific IPFIX Information Elements and lookup functions
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

#include <string.h>
#include <stdio.h>
#include "ipfix_names.h"
#include "ipfix.h"
#include "ipfix_iana.h"
#include "ipfix_iana.c"

#ifdef __cplusplus
extern "C" {
#endif

struct ipfix_identifier ipfixids[] = {
     { PSAMP_TYPEID_ipHeaderPacketSection, PSAMP_LENGTH_ipHeaderPacketSection, 0, "ipHeaderPacketSection", IPFIX_TYPE_octetArray },
     { PSAMP_TYPEID_ipPayloadPacketSection, PSAMP_LENGTH_ipPayloadPacketSection, 0, "ipPayloadPacketSection", IPFIX_TYPE_octetArray },
     { PSAMP_TYPEID_observationTimeSeconds, PSAMP_LENGTH_observationTimeSeconds, 0, "observationTimeSeconds", IPFIX_TYPE_octetArray},
     { PSAMP_TYPEID_observationTimeMilliseconds, PSAMP_LENGTH_observationTimeMilliseconds, 0, "observationTimeMilliseconds", IPFIX_TYPE_octetArray},
     { PSAMP_TYPEID_observationTimeMicroseconds, PSAMP_LENGTH_observationTimeMicroseconds, 0, "observationTimeMicroseconds", IPFIX_TYPE_octetArray },

     /* Vermont PEN */
     { IPFIX_ETYPEID_frontPayload, 0, IPFIX_PEN_vermont, "frontPayload", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_frontPayload, 0, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revFrontPayload", IPFIX_TYPE_octetArray},
     //{ IPFIX_ETYPEID_frontPayload|IPFIX_ETYPE_reverse_bit, 0, IPFIX_PEN_vermont, "revFrontPayload", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_frontPayloadLen, IPFIX_ELENGTH_frontPayloadLen, IPFIX_PEN_vermont, "frontPayloadLen", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_frontPayloadLen, IPFIX_ELENGTH_frontPayloadLen, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revFrontPayloadLen", IPFIX_TYPE_octetArray},
     //{ IPFIX_ETYPEID_frontPayloadLen|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_frontPayloadLen, IPFIX_PEN_vermont, "revFrontPayloadLen", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_maxPacketGap, IPFIX_ELENGTH_maxPacketGap, IPFIX_PEN_vermont, "maxPacketGap", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_maxPacketGap, IPFIX_ELENGTH_maxPacketGap, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revMaxPacketGap", IPFIX_TYPE_octetArray},
     // { IPFIX_ETYPEID_maxPacketGap|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_maxPacketGap, IPFIX_PEN_vermont, "revMaxPacketGap", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_frontPayloadPktCount, IPFIX_ELENGTH_frontPayloadPktCount, IPFIX_PEN_vermont, "frontPayloadPktCount", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_frontPayloadPktCount, IPFIX_ELENGTH_frontPayloadPktCount, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "frontPayloadPktCount", IPFIX_TYPE_octetArray},
     // { IPFIX_ETYPEID_frontPayloadPktCount|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_frontPayloadPktCount, IPFIX_PEN_vermont, "frontPayloadPktCount", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_dpaFlowCount, IPFIX_ELENGTH_dpaFlowCount, IPFIX_PEN_vermont, "dpaFlowCount", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_dpaForcedExport, IPFIX_ELENGTH_dpaForcedExport, IPFIX_PEN_vermont, "dpaForcedExport", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_dpaReverseStart, IPFIX_ELENGTH_dpaReverseStart, IPFIX_PEN_vermont, "dpaReverseStart", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_transportOctetDeltaCount, IPFIX_ELENGTH_transportOctetDeltaCount, IPFIX_PEN_vermont, "transportOctetDeltaCount", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_transportOctetDeltaCount, IPFIX_ELENGTH_transportOctetDeltaCount, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revTransportOctetDeltaCount", IPFIX_TYPE_octetArray},
     // { IPFIX_ETYPEID_transportOctetDeltaCount|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_transportOctetDeltaCount, IPFIX_PEN_vermont, "revTransportOctetDeltaCount", IPFIX_TYPE_octetArray},
     { IPFIX_ETYPEID_anonymisationType, IPFIX_ELENGTH_anonymisationType, IPFIX_PEN_vermont, "anonymisationType", IPFIX_TYPE_octetArray},
};


/* lookup a certain ipfix ID into its name */
const struct ipfix_identifier * ipfix_id_lookup(uint16_t id, uint32_t pen)
{
	uint32_t i;
    // Search IANA IPFIX IDs
    for (i=0; i<sizeof(ipfixids_iana)/sizeof(struct ipfix_identifier); i++) {
        if (ipfixids_iana[i].id==id && ipfixids_iana[i].pen==pen) {
            return &ipfixids_iana[i];
        }
    }
    // Search other IPFIX IDs
	for (i=0; i<sizeof(ipfixids)/sizeof(struct ipfix_identifier); i++) {
		if (ipfixids[i].id==id && ipfixids[i].pen==pen) {
			return &ipfixids[i];
		}
	}
	return NULL;
}

/*
 lookup an ipfix name into its ID
 int because we need -1 for "not found"
 */
const struct ipfix_identifier* ipfix_name_lookup(const char *name)
{
	uint32_t i;

    // Search IANA IPFIX IDs
    for (i=0; i<sizeof(ipfixids_iana)/sizeof(struct ipfix_identifier); i++) {
        if (strcasecmp(name, ipfixids_iana[i].name)==0) {
            return &ipfixids_iana[i];
        }
     }
    // Search other IPFIX IDs
	for (i=0; i<sizeof(ipfixids)/sizeof(struct ipfix_identifier); i++) {
		if (strcasecmp(name, ipfixids[i].name)==0) {
			return &ipfixids[i];
		}
	}

	/* not found */
	return NULL;
}

/*
 * lookup an ipfix semantic by name
 */
const uint8_t* ipfix_semantic_lookup(const char *name)
{
	uint32_t i;

    // Search IANA semantic IDs
    for (i = 0; i<sizeof(ipfixsemantic_iana)/sizeof(struct ipfix_semantic); i++) {
        if (strcasecmp(name, ipfixsemantic_iana[i].name)==0) {
            return &(ipfixsemantic_iana[i].id);
        }
     }

	/* not found */
	return NULL;
}

#ifdef __cplusplus
}
#endif
