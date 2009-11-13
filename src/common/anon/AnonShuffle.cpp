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

#include "AnonShuffle.h"

#include <cstring>

AnonShuffle::AnonShuffle ()
{
}

AnonShuffle::~AnonShuffle ()
{
}

AnonPrimitive::ANON_RESULT AnonShuffle::anonymize (void* buf, unsigned int len)
{
	std::vector<unsigned char> permvector;

	for (unsigned int i=0; i<len; i++) 
		permvector.push_back (*(((unsigned char*) buf) + i));
	
	random_shuffle (permvector.begin (), permvector.end ());

	for (unsigned int i=0; i<len; i++)
		memset ((unsigned char*) buf + i, permvector.at (i), 1);
	
	return ANON_RESULT (len);
}

