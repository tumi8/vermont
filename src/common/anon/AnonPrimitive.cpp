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

#include "AnonPrimitive.h"

AnonPrimitive::AnonPrimitive ()
: next (NULL)
{
}

AnonPrimitive* AnonPrimitive::getNext ()
{
	return next;
}

void AnonPrimitive::setNext (AnonPrimitive* nextprim)
{
	next = nextprim;
}

unsigned int AnonPrimitive::anonimizeBuffer (void* buf, unsigned int len)
{
	ANON_RESULT myres	= anonymize (buf, len);
	ANON_RESULT nextres = myres;

	if (myres.cont && next != NULL) 
		nextres = next->anonimizeBuffer (buf, myres.newlength);

	return std::min(myres.newlength, nextres.newlength);
}

