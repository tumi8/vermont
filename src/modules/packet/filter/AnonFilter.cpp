/*
 * Vermont Anonymization Subsystem
 * Copyright (C) 2008 Lothar Braun
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "AnonFilter.h"
#include "modules/packet/Template.h"

#include <common/anon/AnonModule.h>

void AnonFilter::setIEHeaderOffsets()
{
	// Lookup offset, header, and packet class of anonymized packet fields
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		const InformationElement::IeInfo type = i->first;
		if (!Template::getFieldOffsetAndHeader(type, &(i->second.offset), &(i->second.header), &(i->second.packetClass))) {
			msg(MSG_ERROR, "Unkown or unsupported type id %s detected.", i->first.toString().c_str());
			continue;
		}
		DPRINTF("Save the following packet field for anonymization: setIEHeaderOffset id=%s, offset=%u, header=%u, packetClass=%u",
				i->first.toString().c_str(), i->second.offset, i->second.header, i->second.packetClass);
	}
}


bool AnonFilter::processPacket(Packet* p)
{
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		if((p->classification & i->second.packetClass) == 0)
			continue;

		switch (i->second.header) {
			case HEAD_NETWORK:
				anonField(i->first, p->data.netHeader + i->second.offset);
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
