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

#include "RandomNumberGenerator.h"

void RandomNumberGenerator::init ()
{
	//
	// initialize the random number generator which several 
	// components in pktanon use. take the current time as seed.
	// the seeding is per process.
	//
	
	srand ((unsigned int) time (NULL));
}


unsigned int RandomNumberGenerator::generate ()
{
	static unsigned int val = 0;

#ifdef WIN32
	rand_s (&val);
#else
	val = rand ();
#endif

	return val;
}

unsigned int RandomNumberGenerator::generate (unsigned int rangemin, unsigned int rangemax)
{
	if (rangemax == rangemin) return rangemax;

	unsigned int ret = ((generate () % (rangemax - rangemin)) + rangemin);
	assert (ret >= rangemin && ret <= rangemax);

	return ret;
}


