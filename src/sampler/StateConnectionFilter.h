#ifndef _STATE_CONNECITON_FILTER_H_
#define _STATE_CONNECTION_FILTER_H_

#include <sampler/PacketProcessor.h>
#include <common/BloomFilter.h>

#include <map>
#include <ostream>

class MemStatistics;

class StateConnectionFilter : public PacketProcessor {
public:
	StateConnectionFilter(unsigned timeout, unsigned bytes);
	~StateConnectionFilter();

	bool processPacket(Packet* p, bool connFilterResult);
	virtual bool processPacket(Packet* p);
	
	void setExportControlPackets(bool e) { exportControlPackets = e; }
protected:
	unsigned timeout;
	unsigned exportBytes;
	std::map<QuintupleKey, int> exportList;
	bool exportControlPackets;
};

#endif
