#include "AnonFilter.h"
#include "modules/packet/Template.h"

#include <common/anon/AnonModule.h>

void AnonFilter::setIEHeaderOffsets()
{
	// Lookup offset, header, and packet class of anonymized packet fields
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		if (!Template::getFieldOffsetAndHeader(i->first, &(i->second.offset), &(i->second.header), &(i->second.packetClass))) {
			msg(MSG_ERROR, "Unkown or unsupported type id %i detected.", i->first);
			continue;
		}
		DPRINTF("Save the following packet field for anonymization: setIEHeaderOffset id=%i, offset=%u, header=%u, packetClass=%u", i->first, i->second.offset, i->second.header, i->second.packetClass);
	}
}


bool AnonFilter::processPacket(Packet* p)
{
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		if((p->classification & i->second.packetClass) == 0)
			continue; 

		switch (i->second.header) {
			case HEAD_NETWORK:
				anonField(i->first, p->netHeader + i->second.offset);
				break;
			case HEAD_TRANSPORT:
				anonField(i->first, p->transportHeader + i->second.offset);
				break;
			default:
				msg(MSG_ERROR, "Cannot deal with header type %i", i->second.header);
		}
	}
	return true;
}
