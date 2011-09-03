/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

// data types
#define IPFIX_LENGTH_octet 1
#define IPFIX_LENGTH_unsigned16 2
#define IPFIX_LENGTH_unsigned32 4
#define IPFIX_LENGTH_unsigned64 8
#define IPFIX_LENGTH_ipv4Address 4
#define IPFIX_LENGTH_ipv6Address 8
#define IPFIX_LENGTH_dateTimeSeconds 4
#define IPFIX_LENGTH_variable 65535

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

// standard IDs
#define IPFIX_TYPEID_ipVersion                       60
#define IPFIX_TYPEID_sourceIPv4Address                8
#define IPFIX_TYPEID_sourceIPv6Address               27
#define IPFIX_TYPEID_sourceIPv4Mask                   9
#define IPFIX_TYPEID_sourceIPv6Mask                  29
#define IPFIX_TYPEID_sourceIPv4Prefix                44
#define IPFIX_TYPEID_destinationIPv4Address          12
#define IPFIX_TYPEID_destinationIPv6Address          28
#define IPFIX_TYPEID_destinationIPv4Mask             13
#define IPFIX_TYPEID_destinationIPv6Mask             30
#define IPFIX_TYPEID_destinationIPv4Prefix           45
#define IPFIX_TYPEID_classOfServiceIPv4               5
#define IPFIX_TYPEID_classOfServiceV6               137
#define IPFIX_TYPEID_flowLabelV6                     31
#define IPFIX_TYPEID_identificationV4                54
#define IPFIX_TYPEID_protocolIdentifier               4
#define IPFIX_TYPEID_sourceTransportPort              7
#define IPFIX_TYPEID_destinationTransportPort        11
#define IPFIX_TYPEID_icmpTypeCodeIPv4                32
#define IPFIX_TYPEID_igmpType                        33
#define IPFIX_TYPEID_sourceMacAddress                56
#define IPFIX_TYPEID_mplsLabelStackEntry1            70
#define IPFIX_TYPEID_mplsLabelStackEntry2            71
#define IPFIX_TYPEID_mplsLabelStackEntry3            72
#define IPFIX_TYPEID_mplsLabelStackEntry4            73
#define IPFIX_TYPEID_mplsLabelStackEntry5            74
#define IPFIX_TYPEID_mplsLabelStackEntry6            75
#define IPFIX_TYPEID_mplsLabelStackEntry7            76
#define IPFIX_TYPEID_mplsLabelStackEntry8            77
#define IPFIX_TYPEID_mplsLabelStackEntry9            78
#define IPFIX_TYPEID_mplsLabelStackEntry10           79
#define IPFIX_TYPEID_ipNextHopIPv4Address            15
#define IPFIX_TYPEID_ipNextHopIPv6Address            62
#define IPFIX_TYPEID_ingressInterface                10
#define IPFIX_TYPEID_egressInterface                 14
#define IPFIX_TYPEID_ipNextHopAsNumber              129
#define IPFIX_TYPEID_bgpSourceAsNumber               16
#define IPFIX_TYPEID_bgpDestinationAsNumber          17
#define IPFIX_TYPEID_bgpNextHopAsNumber             128
#define IPFIX_TYPEID_bgpNextHopIPv4Address           18
#define IPFIX_TYPEID_bgpNextHopIPv6Address           63
#define IPFIX_TYPEID_mplsTopLabelType                46
#define IPFIX_TYPEID_mplsTopLabelIPv4Address         47
#define IPFIX_TYPEID_mplsTopLabelIPv6Address        140
#define IPFIX_TYPEID_exporterIPv4Address            130
#define IPFIX_TYPEID_exporterIPv6Address            131
#define IPFIX_TYPEID_minPacketLength                 25
#define IPFIX_TYPEID_maxPacketLength                 26
#define IPFIX_TYPEID_minimumTTL                      52
#define IPFIX_TYPEID_maximumTTL                      53
#define IPFIX_TYPEID_ipv6ExtensionHeaders            64
#define IPFIX_TYPEID_tcpControlBits                   6
#define IPFIX_TYPEID_flowStartSysUpTime              22
#define IPFIX_TYPEID_flowEndSysUpTime                21
#define IPFIX_TYPEID_flowStartSeconds               150
#define IPFIX_TYPEID_flowEndSeconds                 151
#define IPFIX_TYPEID_flowStartMilliSeconds          152
#define IPFIX_TYPEID_flowEndMilliSeconds            153
#define IPFIX_TYPEID_flowStartMicroSeconds          154
#define IPFIX_TYPEID_flowEndMicroSeconds            155
#define IPFIX_TYPEID_flowStartNanoSeconds           156
#define IPFIX_TYPEID_flowEndNanoSeconds             157
#define IPFIX_TYPEID_flowActiveTimeOut               36
#define IPFIX_TYPEID_flowInactiveTimeout             37
#define IPFIX_TYPEID_flowEndReason                  136
#define IPFIX_TYPEID_octetDeltaCount                  1
#define IPFIX_TYPEID_postOctetDeltaCount             23
#define IPFIX_TYPEID_octetTotalCount                 85
#define IPFIX_TYPEID_packetDeltaCount                 2
#define IPFIX_TYPEID_postPacketDeltaCount            24
#define IPFIX_TYPEID_packetTotalCount                86
#define IPFIX_TYPEID_droppedOctetDeltaCount         132
#define IPFIX_TYPEID_droppedOctetTotalCount         134
#define IPFIX_TYPEID_droppedPacketDeltaCount        133
#define IPFIX_TYPEID_droppedPacketTotalCount        135
#define IPFIX_TYPEID_postMCastPacketDeltaCount       19
#define IPFIX_TYPEID_postMCastOctetDeltaCount        20
#define IPFIX_TYPEID_observedFlowTotalCount         163
#define IPFIX_TYPEID_exportedOctetTotalCount         40
#define IPFIX_TYPEID_exportedMessageTotalCount       41
#define IPFIX_TYPEID_exportedFlowTotalCount          42

#define IPFIX_TYPEID_fragmentOffsetIPv4              88
#define IPFIX_TYPEID_ipTimeToLive                   192
#define IPFIX_TYPEID_udpSourcePort                  180
#define IPFIX_TYPEID_udpDestinationPort             181
#define IPFIX_TYPEID_tcpSourcePort                  182
#define IPFIX_TYPEID_tcpDestinationPort             183
#define IPFIX_TYPEID_tcpSequenceNumber              184
#define IPFIX_TYPEID_tcpAcknowledgementNumber       185
#define IPFIX_TYPEID_tcpWindowSize                  186
#define IPFIX_TYPEID_tcpUrgentPointer               187
#define IPFIX_TYPEID_totalLengthIPv4                190

#define IPFIX_TYPEID_basicList                      291
#define IPFIX_TYPEID_subTemplateList                292
#define IPFIX_TYPEID_subTemplateMultiList           293

#define PSAMP_TYPEID_ipHeaderPacketSection          313
#define PSAMP_TYPEID_ipPayloadPacketSection         314
#define PSAMP_TYPEID_observationTimeSeconds         322
#define PSAMP_TYPEID_observationTimeMilliSeconds    323
#define PSAMP_TYPEID_observationTimeMicroSeconds    324


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



#define IPFIX_LENGTH_ipVersion                      IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceIPv4Address              IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_sourceIPv6Address              IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_sourceIPv4Mask                 IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceIPv6Mask                 IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceIPv4Prefix               IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_destinationIPv4Address         IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_destinationIPv6Address         IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_destinationIPv4Mask            IPFIX_LENGTH_octet
#define IPFIX_LENGTH_destinationIPv6Mask            IPFIX_LENGTH_octet
#define IPFIX_LENGTH_destinationIPv4Prefix          IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_classOfServiceIPv4             IPFIX_LENGTH_octet
#define IPFIX_LENGTH_classOfServiceV6               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_flowLabelV6                    IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_identificationV4               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_protocolIdentifier             IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceTransportPort            IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_destinationTransportPort       IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_icmpTypeCodeIPv4               IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_igmpType                       IPFIX_LENGTH_octet
#define IPFIX_LENGTH_sourceMacAddress               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_mplsLabelStackEntry1           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry2           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry3           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry4           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry5           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry6           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry7           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry8           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry9           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_mplsLabelStackEntry10          IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_ipNextHopIPv4Address           IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_ipNextHopIPv6Address           IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_ingressInterface               IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_egressInterface                IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_ipNextHopAsNumber              IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpSourceAsNumber              IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpDestinationAsNumber         IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpNextHopAsNumber             IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_bgpNextHopIPv4Address          IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_bgpNextHopIPv6Address          IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_mplsTopLabelType               IPFIX_LENGTH_octet
#define IPFIX_LENGTH_mplsTopLabelIPv4Address        IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_mplsTopLabelIPv6Address        IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_exporterIPv4Address            IPFIX_LENGTH_ipv4Address
#define IPFIX_LENGTH_exporterIPv6Address            IPFIX_LENGTH_ipv6Address
#define IPFIX_LENGTH_minPacketLength                IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_maxPacketLength                IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_minimumTTL                     IPFIX_LENGTH_octet
#define IPFIX_LENGTH_maximumTTL                     IPFIX_LENGTH_octet
#define IPFIX_LENGTH_ipv6ExtensionHeaders           IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_tcpControlBits                 IPFIX_LENGTH_octet
#define IPFIX_LENGTH_flowStartSysUpTime             IPFIX_LENGTH_dateTimeSeconds
#define IPFIX_LENGTH_flowEndSysUpTime               IPFIX_LENGTH_dateTimeSeconds
#define IPFIX_LENGTH_flowStartSeconds               IPFIX_LENGTH_dateTimeSeconds
#define IPFIX_LENGTH_flowEndSeconds                 IPFIX_LENGTH_dateTimeSeconds
#define IPFIX_LENGTH_flowStartMilliSeconds          IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_flowEndMilliSeconds            IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_flowStartMicroSeconds          IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_flowEndMicroSeconds            IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_flowStartNanoSeconds           IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_flowEndNanoSeconds             IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_flowActiveTimeOut              IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_flowInactiveTimeout            IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_flowEndReason                  IPFIX_LENGTH_octet
#define IPFIX_LENGTH_octetDeltaCount                IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_postOctetDeltaCount            IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_octetDeltaCount                IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_octetTotalCount                IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_packetDeltaCount               IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_postPacketDeltaCount           IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_packetTotalCount               IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedOctetDeltaCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedOctetTotalCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedPacketDeltaCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_droppedPacketTotalCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_postMCastPacketDeltaCount      IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_postMCastOctetDeltaCount       IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_observedFlowTotalCount         IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_exportedOctetTotalCount        IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_exportedMessageTotalCount      IPFIX_LENGTH_unsigned64
#define IPFIX_LENGTH_exportedFlowTotalCount         IPFIX_LENGTH_unsigned64

#define IPFIX_LENGTH_fragmentOffsetIPv4             IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_ipTimeToLive                   IPFIX_LENGTH_octet
#define IPFIX_LENGTH_udpSourcePort                  IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_udpDestinationPort             IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_tcpSourcePort                  IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_tcpDestinationPort             IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_tcpSequenceNumber              IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_tcpAcknowledgementNumber       IPFIX_LENGTH_unsigned32
#define IPFIX_LENGTH_tcpWindowSize                  IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_tcpUrgentPointer               IPFIX_LENGTH_unsigned16
#define IPFIX_LENGTH_totalLengthIPv4                IPFIX_LENGTH_unsigned16

#define PSAMP_LENGTH_ipHeaderPacketSection          IPFIX_LENGTH_variable
#define PSAMP_LENGTH_ipPayloadPacketSection         IPFIX_LENGTH_variable
#define PSAMP_LENGTH_observationTimeSeconds         IPFIX_LENGTH_dateTimeSeconds
#define PSAMP_LENGTH_observationTimeMilliSeconds    IPFIX_LENGTH_unsigned64
#define PSAMP_LENGTH_observationTimeMicroSeconds    IPFIX_LENGTH_unsigned64

// enterprise TYPEIDs
#define IPFIX_ELENGTH_frontPayloadLen				IPFIX_LENGTH_unsigned32
#define IPFIX_ELENGTH_dpaFlowCount					IPFIX_LENGTH_unsigned32
#define IPFIX_ELENGTH_dpaForcedExport				IPFIX_LENGTH_octet
#define IPFIX_ELENGTH_dpaReverseStart				IPFIX_LENGTH_octet
#define IPFIX_ELENGTH_transportOctetDeltaCount		IPFIX_LENGTH_unsigned64
#define IPFIX_ELENGTH_maxPacketGap					IPFIX_LENGTH_unsigned32
#define IPFIX_ELENGTH_frontPayloadPktCount			IPFIX_LENGTH_unsigned32
#define IPFIX_ELENGTH_anonymisationType				IPFIX_LENGTH_octet

#define MAX_MSG_LEN   65536

#endif

#ifdef __cplusplus
}
#endif

