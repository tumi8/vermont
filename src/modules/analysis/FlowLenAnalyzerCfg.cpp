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

#include "FlowLenAnalyzerCfg.h"

#include <sstream>
#include <vector>


FlowLenAnalyzerCfg* FlowLenAnalyzerCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new FlowLenAnalyzerCfg(e);
}

FlowLenAnalyzerCfg::FlowLenAnalyzerCfg(XMLElement* elem)
    : CfgHelper<FlowLenAnalyzer, FlowLenAnalyzerCfg>(elem,"flowLenAnalyzer")
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("flowFilename")) {
			flowFilename = get("flowFilename");
		} else if (e->matches("binFilename")) {
			binFilename = get("binFilename");
		} else if (e->matches("bins")) {
			// we expect a list of comma seperated numbers
			std::stringstream ss(get("bins"));
			uint64_t i; 
			while (ss >> i) {
				bins.push_back(i);
				if (ss.peek() == ',') {
					ss.ignore();
				}
			}
			msg(MSG_INFO, "FlowLenAnalyzer: Using bins: ");
			for (std::vector<uint64_t>::const_iterator j = bins.begin(); j != bins.end(); ++j) {
				msg(MSG_INFO, "%lu", *j);
			}
		} else if (e->matches("next")) { 
			// ignore next
		} else {
			msg(MSG_FATAL, "Unknown FlowLenAnalyzer config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (flowFilename == "") THROWEXCEPTION("FlowLenAnalyzer: Missing flowFilename statement.");
	if (binFilename == "") THROWEXCEPTION("FlowLenAnalyzer: Missing binFilename statement.");
	if (bins.empty()) THROWEXCEPTION("FlowLenAnalyzer: Missing bins statement. ");
}

FlowLenAnalyzerCfg::~FlowLenAnalyzerCfg()
{
}

FlowLenAnalyzer* FlowLenAnalyzerCfg::createInstance()
{
	instance = new FlowLenAnalyzer(flowFilename, binFilename, bins);
	return instance;
}

bool FlowLenAnalyzerCfg::deriveFrom(FlowLenAnalyzerCfg* old)
{
	return false;
}
