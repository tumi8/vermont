/*
 * IPFIX structs and lookup functions
 * Copyright (C) 2014 Oliver Gasser
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

#ifndef IPFIX_NAMES_H
#define IPFIX_NAMES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


struct ipfix_identifier {
	uint16_t id;
    uint16_t length;
    uint32_t pen; // enterprise type-id
	char *name;
	uint16_t type; // IPFIX data type
};

int ipfix_id_rangecheck(int id);
const struct ipfix_identifier* ipfix_id_lookup(uint16_t id, uint32_t pen);
const struct ipfix_identifier* ipfix_name_lookup(const char *name);


#ifdef __cplusplus
}
#endif

#endif
