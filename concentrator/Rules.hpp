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

#ifndef RULES_H
#define RULES_H

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "IpfixParser.hpp"
#include "Rule.hpp"

#define MAX_RULES 255

/**
 * Set of aggregation rules
 */
class Rules {
	public:
		Rules(char* fname);
		~Rules();

		int count;
		Rule* rule[MAX_RULES];
};

int parseModifier(const char* s, Rule::Field::Modifier* modifier);
int parseProtoPattern(char* s, IpfixRecord::Data** fdata, IpfixRecord::FieldInfo::Type::Length* length);
int parseIPv4Pattern(char* s, IpfixRecord::Data** fdata, IpfixRecord::FieldInfo::Type::Length* length);
int parsePortPattern(char* s, IpfixRecord::Data** fdata, IpfixRecord::FieldInfo::Type::Length* length);
int parseTcpFlags(char* s, IpfixRecord::Data** fdata, IpfixRecord::FieldInfo::Type::Length* length);

#endif
