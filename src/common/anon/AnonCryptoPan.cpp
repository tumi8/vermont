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

#include "AnonCryptoPan.h"

#include "common/msg.h"

#include <cstring>

/**
 * expects a fully filled 32 byte key buffer
 */
AnonCryptoPan::AnonCryptoPan (char* _key)
: cryptopan ((const UINT8*)_key)
{
}

AnonCryptoPan::~AnonCryptoPan ()
{
}

AnonPrimitive::ANON_RESULT AnonCryptoPan::anonymize(void* buf, unsigned int len)
{
	// IPv4 addresses are usually 4 bytes long, but Vermont internally handles 5 bytes with 1 byte ip mask
	assert ((len=sizeof(UINT32)) || (len == 5));
	UINT32 orig = 0;
	memcpy (&orig, buf, sizeof (UINT32));
	orig = cryptopan.anonymize (orig);
	memcpy (buf, &orig, sizeof (UINT32));

	return ANON_RESULT (len);
}


