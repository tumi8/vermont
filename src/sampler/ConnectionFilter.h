#ifdef HAVE_CONNECTION_FILTER

#ifndef _CONNECTION_FILTER_H_
#define _CONNECTION_FILTER_H_

#include "PacketProcessor.h"

#include <common/bloom/CountBloomFilter.h>
#include <common/bloom/AgeBloomFilter.h>

class ConnectionFilter : public PacketProcessor {
public:
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize);
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize, unsigned seed);

	virtual bool processPacket(Packet* p);
	void setExportControlPackets(bool e) { exportControlPackets = e; }
protected:
	HashParams hashParams;
	AgeBloomFilter synFilter;
	CountBloomFilter exportFilter;
	AgeBloomFilter connectionFilter;
	unsigned timeout;
	unsigned exportBytes;
	bool exportControlPackets;
};

#endif

#endif
