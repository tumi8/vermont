/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

/** @file
 * IPFIX protocol constants.
 *
 * This function provides constants and functions necessary or useful for interpretation of IPFIX messages.
 *
 */

/* wrapper into ipfixlolib - double data makes no sense */

#include <string.h>
#include "common/ipfixlolib/ipfixlolib.h"
#include "ipfix.h"

#ifdef __cplusplus
extern "C" {
#endif

int string2typeid(const char *s)
{
	return ipfix_name_lookup(s);
}

char* typeid2string(int i)
{
        const struct ipfix_identifier *ix;
	ix=ipfix_id_lookup(i);

	if(!ix) {
		return NULL;
	}

	return ix->name;
}

int string2typelength(const char *s)
{
	const struct ipfix_identifier *ix;

	ix=ipfix_id_lookup(ipfix_name_lookup(s));

	if(!ix) {
		return 0;
	}

	return ix->length;
}

#ifdef __cplusplus
}
#endif

