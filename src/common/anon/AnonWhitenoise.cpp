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

#include "AnonWhitenoise.h"

AnonWhitenoise::AnonWhitenoise (unsigned char strengthval)
: strength (strengthval)
{
}

AnonWhitenoise::~AnonWhitenoise ()
{
}

AnonPrimitive::ANON_RESULT AnonWhitenoise::anonymize (void* buf, unsigned int len)
{
	//  how many bits are in the buffer?
	unsigned int bits = len * 8;
	
	// how many bits of these will we flip? Generate a random number and apply the given strength.
	unsigned int randbits = RandomNumberGenerator::generate (0, bits);
	randbits = (unsigned int) ceil (((double)randbits / 100.0) * (strength * 10));

	// in any case we will flip at least one bit, if we have any bits
	if (bits > 0 && randbits == 0)
		randbits++;

	//
	// now we flip randbits in the buffer at random positions
	//

	unsigned int flipbit;
	unsigned char* byte;
	unsigned char mask;

	for (unsigned int i=0; i<randbits; i++) {
		
		flipbit = RandomNumberGenerator::generate (0, bits - 1);
		byte	= ((unsigned char*) buf) + (flipbit / 8);
		mask	= 1 << (flipbit % 8);

		if ((*byte & mask) == 0) 
			*byte |= mask;	// set bit to 1
		else
			*byte &= ~mask;	// set bit to 0

	} // for (unsigned int i=0; i<randbits; i++)

	return ANON_RESULT (len);
}






