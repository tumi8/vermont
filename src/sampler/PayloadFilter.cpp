#include "PayloadFilter.h"


bool PayloadFilter::processPacket(Packet* p)
{
	if (p->classification & PCLASS_PAYLOAD) {
		// "drop" payload
		p->data_length = p->payload - p->data;
		p->classification ^= PCLASS_PAYLOAD;
	}
	return true;
}
