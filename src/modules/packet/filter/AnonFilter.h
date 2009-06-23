#ifndef _ANON_FILTER_H_
#define _ANON_FILTER_H_

#include "PacketProcessor.h"
#include <common/anon/AnonModule.h>

class AnonFilter : public PacketProcessor, public AnonModule {
public:
	virtual bool processPacket(Packet* p);
};

#endif
