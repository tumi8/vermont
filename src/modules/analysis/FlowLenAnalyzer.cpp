/*
  VERMONT 
 * Copyright (C) 2012 Lothar Braun <braun@net.in.tum.de>
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
 */

#include "FlowLenAnalyzer.h"

#include <errno.h>
#include <string.h>

FlowLenAnalyzer::FlowLenAnalyzer(std::string& f)
	: filename(f)
{
	outstream.open(filename.c_str(), ios_base::out | ios_base::trunc);
	if (not outstream) {
		msg(MSG_FATAL, "FlowLenAnalyzer: Could not open output file \"%s\" for writing: %s", filename.c_str(), strerror(errno));
		THROWEXCEPTION("FlowLenAnalyzer: Could not open output file \"%s\" for writing: %s", filename.c_str(), strerror(errno));

	}
}

FlowLenAnalyzer::~FlowLenAnalyzer()
{
	
}

void FlowLenAnalyzer::onDataRecord(IpfixDataRecord* record)
{

}


std::string FlowLenAnalyzer::getStatistics()
{
	return "";
}

std::string FlowLenAnalyzer::getStatisticsXML()
{
	return "";
}
