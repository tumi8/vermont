#ifndef _PAYLOAD_FILTER_H_
#define _PAYLOAD_FILTER_H_

#include "PacketProcessor.h"

class PayloadFilter : public PacketProcessor {
public:
	virtual bool processPacket(Packet* p);
};

#endif
