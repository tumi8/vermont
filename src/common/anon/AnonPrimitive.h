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

#ifndef __ANON_PRIMITIVE_H
#define __ANON_PRIMITIVE_H

#include <string>

class AnonPrimitive {
public:
	AnonPrimitive();
	virtual	~AnonPrimitive() {};

	unsigned int anonymizeBuffer(void* buf, unsigned int len, bool& anonymized, int *cont=NULL);

	AnonPrimitive* getNext();

protected:

	typedef struct _ANON_RESULT {
		bool cont;	// continue with the next primitive
		unsigned int newlength;	// the new length of the buffer

		_ANON_RESULT () : cont (false), newlength (0)
		{
		}

		_ANON_RESULT (unsigned int _newlength, bool _cont = true)
		{
			cont = _cont;
			newlength = _newlength;
		}

		void operator+ (const struct _ANON_RESULT& right)
		{
			cont &= right.cont;
			newlength = std::min(right.newlength, newlength);
		}

	} ANON_RESULT;

	// anonymize the buffer of length len pointed to by buf
	// return the new length of the buffer in case it has changed
	virtual ANON_RESULT anonymize(void* buf, unsigned int len) = 0;

private:

	AnonPrimitive* next;

};

#endif // __ANON_PRIMITIVE_H

