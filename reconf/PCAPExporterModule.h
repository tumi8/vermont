#ifndef _PCAP_EXPORTER_MODULE_H_
#define _PCAP_EXPORTER_MODULE_H_

#include "reconf/Module.h"

#include <common/msg.h>

#include <string>
#include <pcap.h>

class Packet;

class PCAPExporterModule : public Module, public Destination<Packet*>, public Source<Packet*>
{
public:
	PCAPExporterModule(const std::string& file);
	~PCAPExporterModule();

	virtual void receive(Packet* packet);
	virtual void performStart();
	virtual void performShutdown();

	void setDataLinkType(int type);

private:
	static void* pcapExporterSink(void* data);

	std::string fileName;
	pcap_t* dummy;
	pcap_dumper_t* dumper;
	int link_type;
};

#endif
