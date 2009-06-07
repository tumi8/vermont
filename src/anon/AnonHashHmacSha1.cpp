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

#include "AnonHashHmacSha1.h"

#include <cstring>

AnonHashHmacSha1::AnonHashHmacSha1 (std::string hmackey)
{
	keylen = (unsigned int) hmackey.length ();
	key = new char [keylen + 1];
	
#ifdef WIN32
	strcpy_s(key, keylen+1, hmackey.c_str ());
#else
	strcpy(key, hmackey.c_str ());
#endif

}

AnonHashHmacSha1::~AnonHashHmacSha1 ()
{
	delete [] key;
}

AnonPrimitive::ANON_RESULT AnonHashHmacSha1::anonymize (void* buf, unsigned int len)
{
	HmacSha1::sha1_hmac ((char*) &digest, key, keylen, (char*) buf, len);

	unsigned int remaining	= len;
	unsigned int pos		= 0;

	while (remaining != 0) {
		if (remaining >= SHA1_DIGLEN) {
			memcpy((unsigned char*) buf + pos, digest, SHA1_DIGLEN);
			remaining -= SHA1_DIGLEN;
			pos += SHA1_DIGLEN;
		} else {
			memcpy((unsigned char*) buf + pos, digest, remaining);
			remaining =  0;
			pos += remaining;
		}
		
	} // while (remaining != 0) 

	return ANON_RESULT (len);
}

