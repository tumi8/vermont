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

#include "AnonBytewiseHashHmacSha1.h"

AnonBytewiseHashHmacSha1::AnonBytewiseHashHmacSha1(std::string key)
{
	fillTable (key);
}

AnonBytewiseHashHmacSha1::~AnonBytewiseHashHmacSha1 ()
{
}

void AnonBytewiseHashHmacSha1::fillTable (std::string hmackey)
{
	unsigned char data;
	AnonHashHmacSha1 hashobj (hmackey);

	for (unsigned short i = 0; i<256; i++) {

		data = (unsigned char) i;
		bool anonymized = false;
		hashobj.anonymizeBuffer	(&data, 1, anonymized);
		anonbytes[i] = data;

	} // for (unsigned short i = 0; i<256; i++)
}

