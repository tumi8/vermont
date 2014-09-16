/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPFIX_H
#define IPFIX_H

#include "ipfix_iana.h"

// transport protocol numbers
#define IPFIX_protocolIdentifier_ICMP                 1
#define IPFIX_protocolIdentifier_TCP                  6
#define IPFIX_protocolIdentifier_UDP                 17
#define IPFIX_protocolIdentifier_SCTP               132
#define IPFIX_protocolIdentifier_RAW                255

// reserved Set IDs
#define NetflowV9_SetId_Template  				      0
#define NetflowV9_SetId_OptionsTemplate  			  1
#define IPFIX_SetId_Template                          2
#define IPFIX_SetId_OptionsTemplate                   3
#define IPFIX_SetId_DataTemplate                      4
#define IPFIX_SetId_Data_Start                      256

// enterprise bit
#define IPFIX_ENTERPRISE_TYPE   0x8000

#define PSAMP_TYPEID_ipHeaderPacketSection          313
#define PSAMP_TYPEID_ipPayloadPacketSection         314
#define PSAMP_TYPEID_observationTimeSeconds         322
#define PSAMP_TYPEID_observationTimeMilliseconds    323
#define PSAMP_TYPEID_observationTimeMicroseconds    324


#define IPFIX_PEN_reverse                           29305

/* 
// Private enterprise number of Technische Universitaet Muenchen (19518)
#define IPFIX_PEN_TUM				    19518

// For Vermont, we may use ids in the range [1..1023]
// see http://www.net.in.tum.de/html/ipfix-pen/ or 
// http://www.net.in.tum.de/en/miscellaneous/enterprise-specific-information-elements-for-ipfix/
#define IPFIX_PEN_vermont			    IPFIX_PEN_TUM

// For private Information Elements, we cannot use IPFIX_PEN_reverse to
// indicate the reverse direction. Therefore, we reserve the 10th bit the allocated
// private id range to indicate VERMONT-specific reverse Information Elements.
// (i.e., ids in the range 513 to 1023 represent the reverse direction of ids 1 to 511)
#define IPFIX_ETYPE_reverse_bit			    0x00000200
*/

// not registered at IANA
// within Vermont, we combine this PEN with IPFIX_PEN_reverse
// to declare reverse Vermont-specific IEs. That's why this
// PEN is rather large
// FIXME: maybe there is a better way?
#define IPFIX_PEN_vermont							0x77700000

#define IPFIX_ETYPEID_frontPayload					1
#define IPFIX_ETYPEID_frontPayloadLen				2
#define IPFIX_ETYPEID_maxPacketGap					3
#define IPFIX_ETYPEID_frontPayloadPktCount			4
#define IPFIX_ETYPEID_dpaFlowCount					5
#define IPFIX_ETYPEID_dpaForcedExport				6
#define IPFIX_ETYPEID_dpaReverseStart				7
#define IPFIX_ETYPEID_transportOctetDeltaCount		8
/**
 * the following values are valid for IPFIX_ETYPEID_anonymisationType:
 * 0: the preceding information element in the data record was not anonymised
 * 1: the preceding information element in the data record was anonymised
 */
#define IPFIX_ETYPEID_anonymisationType				9


#define PSAMP_LENGTH_ipHeaderPacketSection          IPFIX_BYTES_string
#define PSAMP_LENGTH_ipPayloadPacketSection         IPFIX_BYTES_string
#define PSAMP_LENGTH_observationTimeSeconds         IPFIX_BYTES_dateTimeSeconds
#define PSAMP_LENGTH_observationTimeMilliseconds    IPFIX_BYTES_unsigned64
#define PSAMP_LENGTH_observationTimeMicroseconds    IPFIX_BYTES_unsigned64

// enterprise TYPEIDs
#define IPFIX_ELENGTH_frontPayloadLen				IPFIX_BYTES_unsigned32
#define IPFIX_ELENGTH_dpaFlowCount					IPFIX_BYTES_unsigned32
#define IPFIX_ELENGTH_dpaForcedExport				IPFIX_BYTES_unsigned8
#define IPFIX_ELENGTH_dpaReverseStart				IPFIX_BYTES_unsigned8
#define IPFIX_ELENGTH_transportOctetDeltaCount		IPFIX_BYTES_unsigned64
#define IPFIX_ELENGTH_maxPacketGap					IPFIX_BYTES_unsigned32
#define IPFIX_ELENGTH_frontPayloadPktCount			IPFIX_BYTES_unsigned32
#define IPFIX_ELENGTH_anonymisationType				IPFIX_BYTES_unsigned8

#define MAX_MSG_LEN   65536

#endif

#ifdef __cplusplus
}
#endif

