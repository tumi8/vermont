#include "HookingFilter.h"
#include "packet_hook.h"

bool HookingFilter::processPacket(const Packet *p)
{
	struct packet_hook ph;

        /* marshall data into C struct - just pointers */
	ph.data=p->data;
	ph.ip_header=p->ipHeader;
	ph.transport_header=p->transportHeader;

	ph.timestamp=&(p->timestamp);
	ph.length=p->length;

        /* call hooking function */
	hook((void *)&ph);

	return true;
}
