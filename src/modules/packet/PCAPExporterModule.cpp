#include "PCAPExporterModule.h"

#include "modules/packet/Packet.h"

PCAPExporterModule::PCAPExporterModule(const std::string& file)
	: fileName(file), dummy(NULL), dumper(NULL), link_type(0), snaplen(PCAP_MAX_CAPTURE_LENGTH)
{
}

PCAPExporterModule::~PCAPExporterModule()
{
}

void PCAPExporterModule::setDataLinkType(int type)
{
	link_type = type;

}

void PCAPExporterModule::setSnaplen(int len)
{
	snaplen = len;
}

void PCAPExporterModule::performStart()
{
	char errbuf[PCAP_ERRBUF_SIZE];
	dummy = pcap_open_dead(link_type, snaplen);
	if (!dummy) {
		THROWEXCEPTION("Could not open dummy device: %s", errbuf);
	}
	dumper = pcap_dump_open(dummy, fileName.c_str());
	if (!dumper) {
		THROWEXCEPTION("Could not open dump file: %s", errbuf);
	}
}

void PCAPExporterModule::performShutdown()
{
	if (dumper) {
		if (-1 == pcap_dump_flush(dumper)) {
			msg(MSG_FATAL, "PCAPExporterModule: Could not flush dump file");
		}
		pcap_dump_close(dumper);
	}
}

void PCAPExporterModule::receive(Packet* packet)
{
	static struct pcap_pkthdr packetHeader;
	packetHeader.ts = packet->timestamp;
	packetHeader.caplen = packet->data_length;
	packetHeader.len = packet->pcapPacketLength;
	pcap_dump((unsigned char*)dumper, &packetHeader, packet->data);
	packet->removeReference();
}

