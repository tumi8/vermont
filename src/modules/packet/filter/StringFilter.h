/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
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
 * Filter a packet by checking if it is containing a predefined string
 */

#ifndef STRINGFILTER_H
#define STRINGFILTER_H

#include <vector>
#include <string>
#include "common/msg.h"
#include "PacketProcessor.h"



class StringFilter : public PacketProcessor
{
public:
	StringFilter();
	virtual ~StringFilter();

	static std::string hexparser(const std::string input);
	virtual bool processPacket (Packet * p);
	void addandFilter(std::string string);
	void addnotFilter(std::string string);

protected:
	std::vector<std::string> andFilters;
	std::vector<std::string> notFilters;

	bool compare (unsigned char *data, std::string toMatch, unsigned int plength);
};

#endif
