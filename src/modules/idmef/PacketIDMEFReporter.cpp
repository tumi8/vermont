#include "PacketIDMEFReporter.h"

#include "modules/ipfix/IpfixRecord.hpp"
#include "common/Misc.h"


const char* PacketIDMEFReporter::PAR_SRCIP = "PacketSourceIP";
const char* PacketIDMEFReporter::PAR_DSTIP = "PacketDestinationIP";
const char* PacketIDMEFReporter::PAR_PROTOCOL = "PacketProtocol";
const char* PacketIDMEFReporter::PAR_LENGTH = "PacketLength";
const char* PacketIDMEFReporter::PAR_SRCPORT = "PacketSourcePort";
const char* PacketIDMEFReporter::PAR_DSTPORT = "PacketDestinationPort";
const char* PacketIDMEFReporter::PAR_SNAPSHOT = "PacketSnapshot";

InstanceManager<IDMEFMessage> PacketIDMEFReporter::idmefManager("IDMEFMessage");



PacketIDMEFReporter::PacketIDMEFReporter(string idmeftemplate, string analyzerid, uint16_t snapshotoff, uint16_t snapshotlen)
	: analyzerId(analyzerid), idmefTemplate(idmeftemplate),
	  snapshotOffset(snapshotoff), snapshotLength(snapshotlen)
{
}

PacketIDMEFReporter::~PacketIDMEFReporter()
{
}

void PacketIDMEFReporter::analyzePacket(Packet* p, IDMEFMessage* msg)
{
	char buffer[20];
	uint16_t i = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(IPFIX_TYPEID_sourceIPv4Address, p);
	uint32_t srcip = *(uint32_t*)(p->netHeader+i);
	msg->setVariable(PAR_SRCIP, IPToString(srcip).c_str());
	i = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(IPFIX_TYPEID_destinationIPv4Address, p);
	uint32_t dstip = *(uint32_t*)(p->netHeader+i);
	msg->setVariable(PAR_DSTIP, IPToString(dstip).c_str());
	i = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(IPFIX_TYPEID_protocolIdentifier, p);
	uint8_t protocol = *(uint8_t*)(p->netHeader+i);
	snprintf(buffer, 20, "%hhu", protocol);
	msg->setVariable(PAR_PROTOCOL, buffer);
	i = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(IPFIX_TYPEID_octetDeltaCount, p);
	uint16_t packetlen = *(uint16_t*)(p->netHeader+i);
	snprintf(buffer, 20, "%hu", packetlen);
	msg->setVariable(PAR_LENGTH, buffer);
	if ((protocol & (Packet::TCP|Packet::UDP))>0) {
		i = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(IPFIX_TYPEID_sourceTransportPort, p);
		uint16_t srcport = *(uint16_t*)(p->netHeader+i);
		snprintf(buffer, 20, "%hu", srcport);
		msg->setVariable(PAR_SRCPORT, srcport);
		i = IpfixRecord::TemplateInfo::getRawPacketFieldIndex(IPFIX_TYPEID_destinationTransportPort, p);
		uint16_t dstport = *(uint16_t*)(p->netHeader+i);
		snprintf(buffer, 20, "%hu", dstport);
		msg->setVariable(PAR_DSTPORT, buffer);
	}
	if (p->payload != NULL) {
		uint16_t len = p->data_length-p->payloadOffset;
		if (len>snapshotLength) len = snapshotLength;
		for (uint16_t i=0; i<len; i++) {
			snprintf(&snapshotBuffer[i*2], 3, "%X", p->payload[i]);
		}
		msg->setVariable(PAR_SNAPSHOT, snapshotBuffer);
	}
}

void PacketIDMEFReporter::receive(Packet* p)
{
	IDMEFMessage* msg = idmefManager.getNewInstance();
	msg->init(idmefTemplate, analyzerId);
	analyzePacket(p, msg);
	msg->applyVariables();
	send(msg);
}
