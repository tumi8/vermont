#include "reconf/PacketSource.h"

#include <stdexcept>

void PacketSource::connectTo(Destination* destination)
{ 
	PacketDestination* d = dynamic_cast<PacketDestination*>(destination);
	if (!d) {
		throw std::runtime_error("error: only PacketDestination could connect to this\n");
	}
	printf("PacketSource::connectTo(Destination*)\n");	
	
	if (dest) 
		throw std::runtime_error("error: already connected\n");
	
	dest = d;
}
