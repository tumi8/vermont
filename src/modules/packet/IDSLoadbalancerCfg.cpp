/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2010 Vermont Project
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

#include "IDSLoadbalancerCfg.h"
#include "BasePacketSelector.hpp"

#include "common/defs.h"

#include <cassert>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>

IDSLoadbalancerCfg::IDSLoadbalancerCfg(XMLElement* elem)
	: CfgHelper<IDSLoadbalancer, IDSLoadbalancerCfg>(elem, "IDSLoadbalancer"),
	_queuecount(1), _selector("")
{
	if (!elem) return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
			it != set.end();
			it++) {
		XMLElement* e = *it;

		if (e->matches("queuecount")) {
			_queuecount = getInt("queuecount", 1, e);
		} else if (e->matches("PacketSelector")) {
			XMLAttribute *a = e->getAttribute("type");
			if (!a)
				THROWEXCEPTION("no PacketSelector specified");
			_selector = a->getValue();

			if(_selector == "HashPacketSelector"){

			} else if (_selector == "IpPacketSelector"){
				msg(MSG_INFO, "IpPacketSelector");
				XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
				for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
						it != set.end();
						it++) {
					XMLElement* e = *it;
					if(e->matches("DestinationIp")){
						XMLAttribute *a = e->getAttribute("queueno");
						if (!a)
							THROWEXCEPTION("No queue number specified");
						int queueno = 0;
						std::string tmp = a->getValue();
						try {
							queueno = boost::lexical_cast<int>(tmp);
						}catch (boost::bad_lexical_cast &){
							THROWEXCEPTION("bad value for queue number: %s", tmp.c_str());
						}
						std::string ip = e->getFirstText();
						dst[parseIp(ip)] = queueno;
					}else if (e->matches("SourceIp")){
						XMLElement* e = *it;
						XMLAttribute *a = e->getAttribute("queueno");
						if (!a)
							THROWEXCEPTION("No queue number specified");
						int queueno = 0;
						std::string tmp = a->getValue();
						try {
							queueno = boost::lexical_cast<int>(tmp);
						}catch (boost::bad_lexical_cast &){
							THROWEXCEPTION("bad value for queue number: %s", tmp.c_str());
						}
						std::string ip = e->getFirstText();
						src[parseIp(ip)] = queueno;
					}
				}
			} else {
				THROWEXCEPTION("Invalid selector: %s", _selector.c_str());
			}
		}
	}
	if (_selector.empty())
		THROWEXCEPTION("No selector specified");
}

IDSLoadbalancerCfg* IDSLoadbalancerCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IDSLoadbalancerCfg(elem);
}

IDSLoadbalancerCfg::~IDSLoadbalancerCfg()
{
}

IDSLoadbalancer* IDSLoadbalancerCfg::createInstance()
{
	instance = new IDSLoadbalancer(_selector, _queuecount);
	if (_selector == "IpPacketSelector"){
		instance->setIpConfig(src, dst);
	}
	return instance;
}

bool IDSLoadbalancerCfg::deriveFrom(IDSLoadbalancerCfg* old)
{
	return true; // FIXME: implement
}


uint32_t IDSLoadbalancerCfg::parseIp(std::string &input){
	struct in_addr inp;
	int res = inet_aton(input.c_str(), &inp);
	if (!res) {
		THROWEXCEPTION("Invalid IP address: %s", input.c_str());
	} 
	return inp.s_addr;
}
