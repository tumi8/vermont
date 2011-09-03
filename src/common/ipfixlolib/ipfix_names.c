#include <string.h>
#include <stdio.h>
#include "ipfix_names.h"
#include "ipfix.h"



struct ipfix_identifier ipfixids[] = {
 { IPFIX_TYPEID_ipVersion, IPFIX_LENGTH_ipVersion, 0, "ipVersion" },
 { IPFIX_TYPEID_sourceIPv4Address, IPFIX_LENGTH_sourceIPv4Address, 0, "sourceIPv4Address" },
 { IPFIX_TYPEID_sourceIPv6Address, IPFIX_LENGTH_sourceIPv6Address, 0, "sourceIPv6Address" },
 { IPFIX_TYPEID_sourceIPv4Mask, IPFIX_LENGTH_sourceIPv4Mask, 0, "sourceIPv4Mask" },
 { IPFIX_TYPEID_sourceIPv6Mask, IPFIX_LENGTH_sourceIPv6Mask, 0, "sourceIPv6Mask" },
 { IPFIX_TYPEID_sourceIPv4Prefix, IPFIX_LENGTH_sourceIPv4Prefix, 0, "sourceIPv4Prefix" },
 { IPFIX_TYPEID_destinationIPv4Address, IPFIX_LENGTH_destinationIPv4Address, 0, "destinationIPv4Address" },
 { IPFIX_TYPEID_destinationIPv6Address, IPFIX_LENGTH_destinationIPv6Address, 0, "destinationIPv6Address" },
 { IPFIX_TYPEID_destinationIPv4Mask, IPFIX_LENGTH_destinationIPv4Mask, 0, "destinationIPv4Mask" },
 { IPFIX_TYPEID_destinationIPv6Mask, IPFIX_LENGTH_destinationIPv6Mask, 0, "destinationIPv6Mask" },
 { IPFIX_TYPEID_destinationIPv4Prefix, IPFIX_LENGTH_destinationIPv4Prefix, 0, "destinationIPv4Prefix" },
 { IPFIX_TYPEID_classOfServiceIPv4, IPFIX_LENGTH_classOfServiceIPv4, 0, "classOfServiceIPv4" },
 { IPFIX_TYPEID_classOfServiceV6, IPFIX_LENGTH_classOfServiceV6, 0, "classOfServiceV6" },
 { IPFIX_TYPEID_flowLabelV6, IPFIX_LENGTH_flowLabelV6, 0, "flowLabelV6" },
 { IPFIX_TYPEID_identificationV4, IPFIX_LENGTH_identificationV4, 0, "identificationV4" },
 { IPFIX_TYPEID_protocolIdentifier, IPFIX_LENGTH_protocolIdentifier, 0, "protocolIdentifier" },
 { IPFIX_TYPEID_sourceTransportPort, IPFIX_LENGTH_sourceTransportPort, 0, "sourceTransportPort" },
 { IPFIX_TYPEID_destinationTransportPort, IPFIX_LENGTH_destinationTransportPort, 0, "destinationTransportPort" },
 { IPFIX_TYPEID_icmpTypeCodeIPv4, IPFIX_LENGTH_icmpTypeCodeIPv4, 0, "icmpTypeCodeIPv4" },
 { IPFIX_TYPEID_igmpType, IPFIX_LENGTH_igmpType, 0, "igmpType" },
 { IPFIX_TYPEID_sourceMacAddress, IPFIX_LENGTH_sourceMacAddress, 0, "sourceMacAddress" },
 { IPFIX_TYPEID_mplsLabelStackEntry1, IPFIX_LENGTH_mplsLabelStackEntry1, 0, "mplsLabelStackEntry1" },
 { IPFIX_TYPEID_mplsLabelStackEntry2, IPFIX_LENGTH_mplsLabelStackEntry2, 0, "mplsLabelStackEntry2" },
 { IPFIX_TYPEID_mplsLabelStackEntry3, IPFIX_LENGTH_mplsLabelStackEntry3, 0, "mplsLabelStackEntry3" },
 { IPFIX_TYPEID_mplsLabelStackEntry4, IPFIX_LENGTH_mplsLabelStackEntry4, 0, "mplsLabelStackEntry4" },
 { IPFIX_TYPEID_mplsLabelStackEntry5, IPFIX_LENGTH_mplsLabelStackEntry5, 0, "mplsLabelStackEntry5" },
 { IPFIX_TYPEID_mplsLabelStackEntry6, IPFIX_LENGTH_mplsLabelStackEntry6, 0, "mplsLabelStackEntry6" },
 { IPFIX_TYPEID_mplsLabelStackEntry7, IPFIX_LENGTH_mplsLabelStackEntry7, 0, "mplsLabelStackEntry7" },
 { IPFIX_TYPEID_mplsLabelStackEntry8, IPFIX_LENGTH_mplsLabelStackEntry8, 0, "mplsLabelStackEntry8" },
 { IPFIX_TYPEID_mplsLabelStackEntry9, IPFIX_LENGTH_mplsLabelStackEntry9, 0, "mplsLabelStackEntry9" },
 { IPFIX_TYPEID_mplsLabelStackEntry10, IPFIX_LENGTH_mplsLabelStackEntry10, 0, "mplsLabelStackEntry10" },
 { IPFIX_TYPEID_ipNextHopIPv4Address, IPFIX_LENGTH_ipNextHopIPv4Address, 0, "ipNextHopIPv4Address" },
 { IPFIX_TYPEID_ipNextHopIPv6Address, IPFIX_LENGTH_ipNextHopIPv6Address, 0, "ipNextHopIPv6Address" },
 { IPFIX_TYPEID_ingressInterface, IPFIX_LENGTH_ingressInterface, 0, "ingressInterface" },
 { IPFIX_TYPEID_egressInterface, IPFIX_LENGTH_egressInterface, 0, "egressInterface" },
 { IPFIX_TYPEID_ipNextHopAsNumber, IPFIX_LENGTH_ipNextHopAsNumber, 0, "ipNextHopAsNumber" },
 { IPFIX_TYPEID_bgpSourceAsNumber, IPFIX_LENGTH_bgpSourceAsNumber, 0, "bgpSourceAsNumber" },
 { IPFIX_TYPEID_bgpDestinationAsNumber, IPFIX_LENGTH_bgpDestinationAsNumber, 0, "bgpDestinationAsNumber" },
 { IPFIX_TYPEID_bgpNextHopAsNumber, IPFIX_LENGTH_bgpNextHopAsNumber, 0, "bgpNextHopAsNumber" },
 { IPFIX_TYPEID_bgpNextHopIPv4Address, IPFIX_LENGTH_bgpNextHopIPv4Address, 0, "bgpNextHopIPv4Address" },
 { IPFIX_TYPEID_bgpNextHopIPv6Address, IPFIX_LENGTH_bgpNextHopIPv6Address, 0, "bgpNextHopIPv6Address" },
 { IPFIX_TYPEID_mplsTopLabelType, IPFIX_LENGTH_mplsTopLabelType, 0, "mplsTopLabelType" },
 { IPFIX_TYPEID_mplsTopLabelIPv4Address, IPFIX_LENGTH_mplsTopLabelIPv4Address, 0, "mplsTopLabelIPv4Address" },
 { IPFIX_TYPEID_mplsTopLabelIPv6Address, IPFIX_LENGTH_mplsTopLabelIPv6Address, 0, "mplsTopLabelIPv6Address" },
 { IPFIX_TYPEID_exporterIPv4Address, IPFIX_LENGTH_exporterIPv4Address, 0, "exporterIPv4Address" },
 { IPFIX_TYPEID_exporterIPv6Address, IPFIX_LENGTH_exporterIPv6Address, 0, "exporterIPv6Address" },
 { IPFIX_TYPEID_minPacketLength, IPFIX_LENGTH_minPacketLength, 0, "minPacketLength" },
 { IPFIX_TYPEID_maxPacketLength, IPFIX_LENGTH_maxPacketLength, 0, "maxPacketLength" },
 { IPFIX_TYPEID_minimumTTL, IPFIX_LENGTH_minimumTTL, 0, "minimumTTL" },
 { IPFIX_TYPEID_maximumTTL, IPFIX_LENGTH_maximumTTL, 0, "maximumTTL" },
 { IPFIX_TYPEID_ipv6ExtensionHeaders, IPFIX_LENGTH_ipv6ExtensionHeaders, 0, "ipv6ExtensionHeaders" },
 { IPFIX_TYPEID_tcpControlBits, IPFIX_LENGTH_tcpControlBits, 0, "tcpControlBits" },
 { IPFIX_TYPEID_tcpControlBits, IPFIX_LENGTH_tcpControlBits, IPFIX_PEN_reverse, "revTcpControlBits" },
 { IPFIX_TYPEID_tcpControlBits, IPFIX_LENGTH_flowStartSysUpTime, 0, "flowStartSysUpTime" },
 { IPFIX_TYPEID_flowEndSysUpTime, IPFIX_LENGTH_flowEndSysUpTime, 0, "flowEndSysUpTime" },
 { IPFIX_TYPEID_flowStartSeconds, IPFIX_LENGTH_flowStartSeconds, 0, "flowStartSeconds" },
 { IPFIX_TYPEID_flowStartSeconds, IPFIX_LENGTH_flowStartSeconds, IPFIX_PEN_reverse, "revFlowStartSeconds" },
 { IPFIX_TYPEID_flowEndSeconds, IPFIX_LENGTH_flowEndSeconds, 0, "flowEndSeconds" },
 { IPFIX_TYPEID_flowEndSeconds, IPFIX_LENGTH_flowEndSeconds, IPFIX_PEN_reverse, "revFlowEndSeconds" },
 { IPFIX_TYPEID_flowStartMilliSeconds, IPFIX_LENGTH_flowStartMilliSeconds, 0, "flowStartMilliSeconds" },
 { IPFIX_TYPEID_flowStartMilliSeconds, IPFIX_LENGTH_flowStartMilliSeconds, IPFIX_PEN_reverse, "revFlowStartMilliSeconds" },
 { IPFIX_TYPEID_flowEndMilliSeconds, IPFIX_LENGTH_flowEndMilliSeconds, 0, "flowEndMilliSeconds" },
 { IPFIX_TYPEID_flowEndMilliSeconds, IPFIX_LENGTH_flowEndMilliSeconds, IPFIX_PEN_reverse, "revFlowEndMilliSeconds" },
 { IPFIX_TYPEID_flowStartMicroSeconds, IPFIX_LENGTH_flowStartMicroSeconds, 0, "flowStartMicroSeconds" },
 { IPFIX_TYPEID_flowStartMicroSeconds, IPFIX_LENGTH_flowStartMicroSeconds, IPFIX_PEN_reverse, "revFlowStartMicroSeconds" },
 { IPFIX_TYPEID_flowEndMicroSeconds, IPFIX_LENGTH_flowEndMicroSeconds, 0, "flowEndMicroSeconds" },
 { IPFIX_TYPEID_flowEndMicroSeconds, IPFIX_LENGTH_flowEndMicroSeconds, IPFIX_PEN_reverse, "revFlowEndMicroSeconds" },
 { IPFIX_TYPEID_flowStartNanoSeconds, IPFIX_LENGTH_flowStartNanoSeconds, 0, "flowStartNanoSeconds" },
 { IPFIX_TYPEID_flowStartNanoSeconds, IPFIX_LENGTH_flowStartNanoSeconds, IPFIX_PEN_reverse, "revFlowStartNanoSeconds" },
 { IPFIX_TYPEID_flowEndNanoSeconds, IPFIX_LENGTH_flowEndNanoSeconds, 0, "flowEndNanoSeconds" },
 { IPFIX_TYPEID_flowEndNanoSeconds, IPFIX_LENGTH_flowEndNanoSeconds, IPFIX_PEN_reverse, "revFlowEndNanoSeconds" },
 { IPFIX_TYPEID_flowActiveTimeOut, IPFIX_LENGTH_flowActiveTimeOut, 0, "flowActiveTimeOut" },
 { IPFIX_TYPEID_flowInactiveTimeout, IPFIX_LENGTH_flowInactiveTimeout, 0, "flowInactiveTimeout" },
 { IPFIX_TYPEID_flowEndReason, IPFIX_LENGTH_flowEndReason, 0, "flowEndReason" },
 { IPFIX_TYPEID_octetDeltaCount, IPFIX_LENGTH_octetDeltaCount, 0, "octetDeltaCount" },
 { IPFIX_TYPEID_octetDeltaCount, IPFIX_LENGTH_octetDeltaCount, IPFIX_PEN_reverse, "revOctetDeltaCount" },
 { IPFIX_TYPEID_postOctetDeltaCount, IPFIX_LENGTH_postOctetDeltaCount, 0, "postOctetDeltaCount" },
 { IPFIX_TYPEID_octetTotalCount, IPFIX_LENGTH_octetTotalCount, 0, "octetTotalCount" },
 { IPFIX_TYPEID_packetDeltaCount, IPFIX_LENGTH_packetDeltaCount, 0, "packetDeltaCount" },
 { IPFIX_TYPEID_packetDeltaCount, IPFIX_LENGTH_packetDeltaCount, IPFIX_PEN_reverse, "revPacketDeltaCount" },
 { IPFIX_TYPEID_postPacketDeltaCount, IPFIX_LENGTH_postPacketDeltaCount, 0, "postPacketDeltaCount" },
 { IPFIX_TYPEID_packetTotalCount, IPFIX_LENGTH_packetTotalCount, 0, "packetTotalCount" },
 { IPFIX_TYPEID_droppedOctetDeltaCount, IPFIX_LENGTH_droppedOctetDeltaCount, 0, "droppedOctetDeltaCount" },
 { IPFIX_TYPEID_droppedOctetTotalCount, IPFIX_LENGTH_droppedOctetTotalCount, 0, "droppedOctetTotalCount" },
 { IPFIX_TYPEID_droppedPacketDeltaCount, IPFIX_LENGTH_droppedPacketDeltaCount, 0, "droppedPacketDeltaCount" },
 { IPFIX_TYPEID_droppedPacketTotalCount, IPFIX_LENGTH_droppedPacketTotalCount, 0, "droppedPacketTotalCount" },
 { IPFIX_TYPEID_postMCastPacketDeltaCount, IPFIX_LENGTH_postMCastPacketDeltaCount, 0, "postMCastPacketDeltaCount" },
 { IPFIX_TYPEID_postMCastOctetDeltaCount, IPFIX_LENGTH_postMCastOctetDeltaCount, 0, "postMCastOctetDeltaCount" },
 { IPFIX_TYPEID_observedFlowTotalCount, IPFIX_LENGTH_observedFlowTotalCount, 0, "observedFlowTotalCount" },
 { IPFIX_TYPEID_exportedOctetTotalCount, IPFIX_LENGTH_exportedOctetTotalCount, 0, "exportedOctetTotalCount" },
 { IPFIX_TYPEID_exportedMessageTotalCount, IPFIX_LENGTH_exportedMessageTotalCount, 0, "exportedMessageTotalCount" },
 { IPFIX_TYPEID_exportedFlowTotalCount, IPFIX_LENGTH_exportedFlowTotalCount, 0, "exportedFlowTotalCount" },
 { IPFIX_TYPEID_fragmentOffsetIPv4, IPFIX_LENGTH_fragmentOffsetIPv4, 0, "fragmentOffsetIPv4" },
 { IPFIX_TYPEID_ipTimeToLive, IPFIX_LENGTH_ipTimeToLive, 0, "ipTimeToLive" },
 { IPFIX_TYPEID_udpSourcePort, IPFIX_LENGTH_udpSourcePort, 0, "udpSourcePort" },
 { IPFIX_TYPEID_udpDestinationPort, IPFIX_LENGTH_udpDestinationPort, 0, "udpDestinationPort" },
 { IPFIX_TYPEID_tcpSourcePort, IPFIX_LENGTH_tcpSourcePort, 0, "tcpSourcePort" },
 { IPFIX_TYPEID_tcpDestinationPort, IPFIX_LENGTH_tcpDestinationPort, 0, "tcpDestinationPort" },
 { IPFIX_TYPEID_tcpSequenceNumber, IPFIX_LENGTH_tcpSequenceNumber, 0, "tcpSequenceNumber" },
 { IPFIX_TYPEID_tcpAcknowledgementNumber, IPFIX_LENGTH_tcpAcknowledgementNumber, 0, "tcpAcknowledgementNumber" },
 { IPFIX_TYPEID_tcpWindowSize, IPFIX_LENGTH_tcpWindowSize, 0, "tcpWindowSize" },
 { IPFIX_TYPEID_tcpUrgentPointer, IPFIX_LENGTH_tcpUrgentPointer, 0, "tcpUrgentPointer" },
 { IPFIX_TYPEID_totalLengthIPv4, IPFIX_LENGTH_totalLengthIPv4, 0, "totalLengthIPv4" },
 { PSAMP_TYPEID_ipHeaderPacketSection, PSAMP_LENGTH_ipHeaderPacketSection, 0, "ipHeaderPacketSection" },
 { PSAMP_TYPEID_ipPayloadPacketSection, PSAMP_LENGTH_ipPayloadPacketSection, 0, "ipPayloadPacketSection" },
 { PSAMP_TYPEID_observationTimeSeconds, PSAMP_LENGTH_observationTimeSeconds, 0, "observationTimeSeconds" },
 { PSAMP_TYPEID_observationTimeMilliSeconds, PSAMP_LENGTH_observationTimeMilliSeconds, 0, "observationTimeMilliSeconds" },
 { PSAMP_TYPEID_observationTimeMicroSeconds, PSAMP_LENGTH_observationTimeMicroSeconds, 0, "observationTimeMicroSeconds" },
 { IPFIX_ETYPEID_frontPayload, 0, IPFIX_PEN_vermont, "frontPayload" },
 { IPFIX_ETYPEID_frontPayload, 0, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revFrontPayload" },
//{ IPFIX_ETYPEID_frontPayload|IPFIX_ETYPE_reverse_bit, 0, IPFIX_PEN_vermont, "revFrontPayload" },
 { IPFIX_ETYPEID_frontPayloadLen, IPFIX_ELENGTH_frontPayloadLen, IPFIX_PEN_vermont, "frontPayloadLen" },
 { IPFIX_ETYPEID_frontPayloadLen, IPFIX_ELENGTH_frontPayloadLen, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revFrontPayloadLen" },
//{ IPFIX_ETYPEID_frontPayloadLen|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_frontPayloadLen, IPFIX_PEN_vermont, "revFrontPayloadLen" },
 { IPFIX_ETYPEID_maxPacketGap, IPFIX_ELENGTH_maxPacketGap, IPFIX_PEN_vermont, "maxPacketGap" },
 { IPFIX_ETYPEID_maxPacketGap, IPFIX_ELENGTH_maxPacketGap, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revMaxPacketGap" },
// { IPFIX_ETYPEID_maxPacketGap|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_maxPacketGap, IPFIX_PEN_vermont, "revMaxPacketGap" },
 { IPFIX_ETYPEID_frontPayloadPktCount, IPFIX_ELENGTH_frontPayloadPktCount, IPFIX_PEN_vermont, "frontPayloadPktCount" },
 { IPFIX_ETYPEID_frontPayloadPktCount, IPFIX_ELENGTH_frontPayloadPktCount, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "frontPayloadPktCount" },
// { IPFIX_ETYPEID_frontPayloadPktCount|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_frontPayloadPktCount, IPFIX_PEN_vermont, "frontPayloadPktCount" },
 { IPFIX_ETYPEID_dpaFlowCount, IPFIX_ELENGTH_dpaFlowCount, IPFIX_PEN_vermont, "dpaFlowCount" },
 { IPFIX_ETYPEID_dpaForcedExport, IPFIX_ELENGTH_dpaForcedExport, IPFIX_PEN_vermont, "dpaForcedExport" },
 { IPFIX_ETYPEID_dpaReverseStart, IPFIX_ELENGTH_dpaReverseStart, IPFIX_PEN_vermont, "dpaReverseStart" },
 { IPFIX_ETYPEID_transportOctetDeltaCount, IPFIX_ELENGTH_transportOctetDeltaCount, IPFIX_PEN_vermont, "transportOctetDeltaCount" },
 { IPFIX_ETYPEID_transportOctetDeltaCount, IPFIX_ELENGTH_transportOctetDeltaCount, IPFIX_PEN_vermont|IPFIX_PEN_reverse, "revTransportOctetDeltaCount" },
// { IPFIX_ETYPEID_transportOctetDeltaCount|IPFIX_ETYPE_reverse_bit, IPFIX_ELENGTH_transportOctetDeltaCount, IPFIX_PEN_vermont, "revTransportOctetDeltaCount" },
 { IPFIX_ETYPEID_anonymisationType, IPFIX_ELENGTH_anonymisationType, IPFIX_PEN_vermont, "anonymisationType" },
 { IPFIX_TYPEID_basicList, 0, 0, "basicList" },
 { IPFIX_TYPEID_subTemplateList, 0, 0, "subTemplateList" },
 { IPFIX_TYPEID_subTemplateMultiList, 0, 0, "subTemplateMultiList" },
};


/* lookup a certain ipfix ID into its name */
const struct ipfix_identifier * ipfix_id_lookup(uint16_t id, uint32_t pen)
{
	uint32_t i;
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

	for (i=0; i<sizeof(ipfixids)/sizeof(struct ipfix_identifier); i++) {
		if (strcasecmp(name, ipfixids[i].name)==0) {
			return &ipfixids[i];
		}
	}

	/* not found */
	return NULL;
}

#ifdef __cplusplus
}
#endif
