#ifndef _PCAP_EXPORTER_CONFIGURATION_H_
#define _PCAP_EXPORTER_CONFIGURATION_H_


#include "ipfix_configuration.h"



#include <vector>


class PcapExporterSink;


class PcapExporterConfiguration : public Configuration {
public:
	PcapExporterConfiguration(xmlDocPtr document, xmlNodePtr startPoint);
	~PcapExporterConfiguration();
	
	virtual void configure();
	virtual void connect(Configuration*);
	virtual void startSystem();
	virtual void stopSystem();

	PcapExporterSink* getExporterSink() const { return exporterSink; }
	
protected:
	void setUp();

private:
	PcapExporterSink* exporterSink;
	std::string fileName;
};

#endif
