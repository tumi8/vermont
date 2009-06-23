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

#include "./AnonHashSha1.h"

#include <cstring>

AnonHashSha1::AnonHashSha1 ()
{
}

AnonHashSha1::~AnonHashSha1 ()
{
}
			
AnonPrimitive::ANON_RESULT AnonHashSha1::anonymize (void* buf, unsigned int len)
{
	Sha1::SHA1_Init		(&sha);
	Sha1::SHA1_Update	(&sha, (unsigned char*) buf, len);
	Sha1::SHA1_Final	((unsigned char*) &digest, &sha);

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

