/*
 * NetflowV9 to IPFIX Converter Module
 * Copyright (C) 2009 Gerhard Muenz
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

#include "NetflowV9ConverterCfg.hpp"


NetflowV9ConverterCfg* NetflowV9ConverterCfg::create(XMLElement* e)
{
    assert(e);
    assert(e->getName() == getName());
    return new NetflowV9ConverterCfg(e);
}

NetflowV9ConverterCfg::NetflowV9ConverterCfg(XMLElement* elem)
    : CfgHelper<NetflowV9Converter, NetflowV9ConverterCfg>(elem, "netflowV9Converter"),
    copyMode(true), keepFlowSysUpTime(false)
{
    if (!elem) return;

    XMLNode::XMLSet<XMLElement*> set = _elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("copyMode")) {
			copyMode = getBool("copyMode");
		} else if (e->matches("keepFlowSysUpTime")) {
			keepFlowSysUpTime = getBool("keepFlowSysUpTime");
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unknown NetflowV9Converter config statement %s\n", e->getName().c_str());
			continue;
		}
	}
	if (keepFlowSysUpTime && !copyMode) {
		THROWEXCEPTION("Cannot configure keepFlowSysUpTime without copyMode!");
	}
}

NetflowV9ConverterCfg::~NetflowV9ConverterCfg()
{
}

NetflowV9Converter* NetflowV9ConverterCfg::createInstance()
{
    instance = new NetflowV9Converter(copyMode, keepFlowSysUpTime);
    return instance;
}

bool NetflowV9ConverterCfg::deriveFrom(NetflowV9ConverterCfg* old)
{
    return false;
}

