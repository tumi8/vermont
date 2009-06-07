#include "AnonFilter.h"
#include "Template.h"

#include <anon/AnonModule.h>

bool AnonFilter::processPacket(Packet* p)
{
	static uint16_t offset;
	static unsigned short header;
	static unsigned long packetClass;

	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		if (!Template::getFieldOffsetAndHeader(i->first, &offset, &header, &packetClass)) {
			msg(MSG_ERROR, "Unkown or unsupported type id %i detected.", i->first);
			continue;
		}

		switch (header) {
		case HEAD_NETWORK:
			anonField(i->first, p->netHeader + offset);
			break;
		case HEAD_TRANSPORT:
			anonField(i->first, p->transportHeader + offset);
			break;
		default:
			msg(MSG_ERROR, "Cannot deal with header type %i", header);
		}
	}
	return true;
}
