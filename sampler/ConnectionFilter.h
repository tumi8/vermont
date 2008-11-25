#ifdef HAVE_CONNECTION_FILTER

#ifndef _CONNECTION_FILTER_H_
#define _CONNECTION_FILTER_H_

#include "PacketProcessor.h"

#include <common/CountBloomFilter.h>
#include <common/AgeBloomFilter.h>

class ConnectionFilter : public PacketProcessor {
public:
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize);
	ConnectionFilter(unsigned timeout, unsigned bytes, unsigned hashFunctions, unsigned FilterSize, unsigned seed);


	virtual bool processPacket(Packet* p);
protected:
	HashParams hashParams;
	AgeBloomFilter synFilter;
	CountBloomFilter exportFilter;
	AgeBloomFilter connectionFilter;
	unsigned timeout;
	unsigned exportBytes;
};

#endif

#endif
