#include "PacketDestination.h"
#include <stdexcept>

inline void recvPacket(Packet* p) {
	printf("recv called\n");
}

void PacketDestination::receive(Emitable* e)
{
	printf("Emitable received\n");
	Packet *p = dynamic_cast<Packet *>(e);
	if (!e) {
		throw std::runtime_error("PacketDestination only could receive Packets*\n");
	}
	receive(p);
}

/*
void PacketDestination::receive(Packet* p)
{
	printf("recv called\n");
}
*/

