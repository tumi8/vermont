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

		if (e->matches("filename")) {
			filename = get("filename");
		} else if (e->matches("next")) { 
			// ignore next
		} else {
			msg(MSG_FATAL, "Unknown FlowLenAnalyzer config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (filename == "") THROWEXCEPTION("FlowLenAnalyzer: Missing filename.");
}

FlowLenAnalyzerCfg::~FlowLenAnalyzerCfg()
{
}

FlowLenAnalyzer* FlowLenAnalyzerCfg::createInstance()
{
	instance = new FlowLenAnalyzer(filename);
	return instance;
}

bool FlowLenAnalyzerCfg::deriveFrom(FlowLenAnalyzerCfg* old)
{
	return false;
}
