#include "HookingFilter.h"
#include "packet_hook.h"
#include "Packet.h"

bool HookingFilter::processPacket(const Packet *p)
{
	struct packet_hook ph;

	/* we want only IPv4 packets  */
	if((p->classification & PCLASS_NET_IP4) == 0)
	    return true;
	    
        /* marshall data into C struct - just pointers */
	ph.data=p->data;
	ph.ip_header=p->netHeader;
	ph.transport_header=p->transportHeader;

	ph.timestamp=&(p->timestamp);
	ph.length=p->length;

        /* call hooking function */
	f(ctx, (void *)&ph);

	return true;
}
