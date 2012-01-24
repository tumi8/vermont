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

#ifndef _FLOWLEN_ANALYZER_H_
#define _FLOWLEN_ANALYZER_H_

#include "modules/ipfix/IpfixRecordDestination.h"

#include <fstream>
#include <map>

class FlowLenAnalyzer : public Module, public Source<NullEmitable*>, public IpfixRecordDestination {
public:
	FlowLenAnalyzer(std::string& flowFilename, std::string& binFilename, std::vector<uint64_t>& bins);
	virtual ~FlowLenAnalyzer();

	virtual void onDataRecord(IpfixDataRecord* record);

private:
	virtual std::string getStatistics();
	virtual std::string getStatisticsXML();

	std::string flowFilename;
	std::ofstream flowOutstream;
	std::string binsFilename;
	std::ofstream binsOutstream;
	std::vector<uint64_t> bins;
	std::map<uint64_t, uint64_t> binStats;
};

#endif
