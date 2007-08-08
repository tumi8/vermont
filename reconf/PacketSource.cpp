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

bool PacketSource::isConnected() const
{
	/* FIXME: should this put inside a mutex to prevent race conditions?
	 *
	 *	  On the other hand, a caller must handle race conditions, because
	 *	  nothing prevents another thread to unconnect source from dest after
	 *	  returning true in this method
	 */
	return dest != NULL;
}

void PacketSource::disconnect()
{
	/* FIXME: should this be inside a mutex to prevent race conditions? */
	dest = NULL;
}
