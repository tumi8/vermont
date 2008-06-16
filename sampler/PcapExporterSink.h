#ifndef _PCAP_EXPORTER_SINK_H_
#define _PCAP_EXPORTER_SINK_H_

#include "Sink.h"

#include <common/msg.h>
#include <common/Thread.h>

#include <string>
#include <pcap.h>

class PcapExporterSink : public Sink
{
public:
	PcapExporterSink(const std::string& file);
	~PcapExporterSink();

        void runSink()
        {
                msg(MSG_DEBUG, "PcapExporterSink: now starting ExporterSink thread");
                thread.run(this);
        };

        bool terminateSink()
        {
                exitFlag = true;
                msg(MSG_DEBUG, "PcapExporterSink: waiting for exporter thread");
                thread.join();
                msg(MSG_DEBUG, "PcapExporterSink: exporter thread joined");
		if (dumper) {
			if (-1 == pcap_dump_flush(dumper)) {
				msg(MSG_FATAL, "PcapExporterSink: Could not flush dump file");
			}
			pcap_dump_close(dumper);
		}

		return true;
        };

	void setDataLinkType(int type);



private:
	static void* pcapExporterSink(void* data);

	Thread thread;
	std::string fileName;
	bool exitFlag;
	pcap_t* dummy;
	pcap_dumper_t* dumper;
};

#endif
