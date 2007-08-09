#include "PacketConnectionSplicer.h"

#include <stdexcept>

PacketConnectionSplicer::PacketConnectionSplicer()
	: mutex()
{
	destinations.reserve(10);
	size = 0;	
}

PacketConnectionSplicer::~PacketConnectionSplicer()
{
}

void PacketConnectionSplicer::connectTo(Destination* destination)
{
	PacketDestination* d = dynamic_cast<PacketDestination*>(destination);
	if (!d) {
		throw std::runtime_error("error: only PacketDestination could connect to PacketConnectionSplicer\n");
	}
	printf("PacketConnectionSplicer::connectTo(Destination*)\n");	
	
	mutex.lock();
	
	// vector will propably reallocate its memory, which could mess up
	// the process function
	if (destinations.capacity() == size)
		throw std::runtime_error("PacketConnectionSplicer: can't add another destination\n");
	
	destinations.push_back(d);
	size = destinations.size();
	
	mutex.unlock();
}

bool PacketConnectionSplicer::isConnected() const
{
	return destinations.size() > 0;
}

void PacketConnectionSplicer::receive(Packet* packet)
{
	printf("PacketConnectionSplicer::receive(Packet*)\n");
	
	// this is in an own function to later have the possibility to process
	// the packets in an own thread; for now, I think this is not needed
	process(packet);
}

inline void PacketConnectionSplicer::process(Packet *packet)
{
	/* we are not using an iterator because this function is a hot path and
	 * we would need a mutex here.
	 * By processing the vector over a mutex proctected variable size
	 * it should be safe. In the worst case, we don't push the packet to a
 	 * newly added PacketDestination
	 */
	vector<PacketDestination*>::size_type sz = size;
	if (sz > 1) // using this strange construct because size-1 could wrap
		packet->addReference(sz - 1);

	vector<PacketDestination*>::size_type i;
	for (i = 0; i < sz; i++) {
		destinations[i]->receive(packet);
	}
}
