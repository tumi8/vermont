//
// Copyright (C) 2008 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#include "AnonBroadcastHandler.h"

AnonBroadcastHandler::AnonBroadcastHandler ()
{
}

AnonBroadcastHandler::~AnonBroadcastHandler ()
{
}

AnonPrimitive::ANON_RESULT AnonBroadcastHandler::anonymize (void* buf, unsigned int len)
{
	//
	// Note: this could also be done using memcmp and a big, static buffer
	// that has been filled with 0xFF at init. But memcmp is often
	// implemented as a loop checking every byte for 0xFF. So this
	// sould make no difference here 
	// (see http://lxr.linux.no/source/arch/m68k/lib/string.c#L228)
	//

	for (unsigned int i=0; i<len; i++)
		if (*((unsigned char*)buf + i) != 0xFF) return ANON_RESULT (len, true);

	//
	// tell the AnonPrimitive handler not to 
	// continue with anonymization such that the
	// broadcast address will not be touched
	// This way we preserve broadcast addresses
	// from anonymization
	//

	return ANON_RESULT (len, false);
}

