#include "PcapExporterSink.h"

PcapExporterSink::PcapExporterSink(const std::string& fileName)
	: Sink("PcapExporterSink"), thread(PcapExporterSink::pcapExporterSink), fileName(fileName),
	exitFlag(false)
{
}

PcapExporterSink::~PcapExporterSink()
{
	msg(MSG_DEBUG, "PcapExporterSink: destructor called");
}

void PcapExporterSink::setDataLinkType(int type)
{
	char errbuf[PCAP_ERRBUF_SIZE];
	dummy = pcap_open_dead(type, 65535);
	if (!dummy) {
		THROWEXCEPTION("Could not open dummy device: %s", errbuf);
	}
}

void* PcapExporterSink::pcapExporterSink(void* data)
{
	PcapExporterSink* sink = static_cast<PcapExporterSink*>(data);
	ConcurrentQueue<Packet*> *queue = sink->getQueue();
	Packet* p = NULL;
	bool result;
	pcap_pkthdr packetHeader;
	char errbuf[PCAP_ERRBUF_SIZE];

	sink->dumper = pcap_dump_open(sink->dummy, sink->fileName.c_str());
	if (!sink->dumper) {
		THROWEXCEPTION("Could not open dump file: %s", errbuf);
	}

	while (!sink->exitFlag) {
		result = queue->pop(&p);
		if (result) {
			packetHeader.ts = p->timestamp;
			packetHeader.caplen = p->data_length;
			packetHeader.len = p->pcap_packet_length;
			pcap_dump((unsigned char*)sink->dumper, &packetHeader, p->data);
		}
		p->removeReference();
	}

	return NULL;
}
