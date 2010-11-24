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
#include "idsloadbalancer/BasePacketSelector.hpp"
#include "idsloadbalancer/PriorityPacketSelector.hpp"
#include "common/defs.h"

#include <cassert>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>

bool compareDecrMask(PriorityNetConfig& pnc1, PriorityNetConfig& pnc2)
{
	return pnc1.mask>pnc2.mask;
}

IDSLoadbalancerCfg::IDSLoadbalancerCfg(XMLElement* elem)
	: CfgHelper<IDSLoadbalancer, IDSLoadbalancerCfg>(elem, "IDSLoadbalancer"),
	_queuecount(1), selector(NULL)
{
	if (!elem) return;

	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
		XMLElement* e = *it;

		if (e->matches("queuecount")) {
			_queuecount = getInt("queuecount", 1, e);
		} else if (e->matches("updateinterval")) {
			updateInterval = getInt("updateinterval", 0, e);
		} else if (e->matches("PacketSelector")) {
			XMLAttribute *a = e->getAttribute("type");
			if (!a)
				THROWEXCEPTION("no PacketSelector specified");
			string _selector = a->getValue();

			if (_selector == "HashPacketSelector") {
				if (!selector) {
					selector = new HashPacketSelector();
				} else
					THROWEXCEPTION("IDSLoadBalancerCfg: multiple packet selectors specified! This is not allowed.");
			} else if (_selector == "IpPacketSelector") {
				msg(MSG_DEBUG, "IpPacketSelector");
				XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
				for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
					XMLElement* e = *it;
					if (e->matches("DestinationIp")){
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
				if (!selector) {
					selector = new IpPacketSelector();
					if (src.empty() && dst.empty())
						THROWEXCEPTION("IDSLoadBalancerCfg: packet selector IpPacketSelector was defined, but no source or destination IPs!");
				} else
					THROWEXCEPTION("IDSLoadBalancerCfg: multiple packet selectors specified! This is not allowed.");

			} else if (_selector == "PriorityPacketSelector") {
				float startprio = getDouble("startPriority", 1.0, e);
				uint32_t minmontime = getInt("minimumMonitoringTime", 10000, e);
				list<PriorityNetConfig> config;
				XMLNode::XMLSet<XMLElement*> set = e->getElementChildren();
				for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
					XMLElement* e = *it;
					if (e->matches("networks")) {
						XMLNode::XMLSet<XMLElement*> netset = e->getElementChildren();
						for (XMLNode::XMLSet<XMLElement*>::iterator nit = netset.begin(); nit != netset.end(); nit++) {
							XMLElement* e = *nit;
							if (e->matches("network")) {
								XMLAttribute* a = e->getAttribute("address");
								if (!a) THROWEXCEPTION("IDSLoadBalancerCfg: no attribute 'address' in configuration element 'network'!");
								string cidr = a->getFirstText();
								size_t pos = cidr.find("/");
								string ip = cidr.substr(0, pos);
								string sbits = cidr.substr(pos+1);
								int maskbits = atoi(sbits.c_str());
								if (maskbits<0 || maskbits>32)
									THROWEXCEPTION("IDSLoadBalancerCfg: attribute 'address' has invalid number of mask bits in configuration (%s)!", sbits.c_str());
								in_addr_t ipaddr = inet_addr(ip.c_str());
								if (ipaddr==(in_addr_t)-1)
									THROWEXCEPTION("IDSLoadBalancerCfg: attribute 'address' has invalid ip subnet in configuration (%s)!", ip.c_str());
								a = e->getAttribute("weight");
								if (!a) THROWEXCEPTION("IDSLoadBalancerCfg: no attribute 'weight' in configuration element 'network'!");
								char* res;
								float weight = strtof(a->getFirstText().c_str(), &res);
								if (weight<=0 || res==a->getFirstText().c_str())
									THROWEXCEPTION("IDSLoadBalancerCfg: attribute 'weight' in configuration element 'network' contains invalid value (%s)!", a->getFirstText().c_str());
								config.push_back(PriorityNetConfig(ntohl((uint32_t)ipaddr), ((1<<(32-maskbits))-1)^0xFFFFFFFF, maskbits, weight));
							}
						}
					}
				}

				if (!selector) {
					struct timeval tv;
					tv.tv_sec = minmontime/1000;
					tv.tv_usec = (minmontime%1000)*1000;

					// sort the network configuration by decreasing maskbits
					config.sort(compareDecrMask);
					selector = new PriorityPacketSelector(config, startprio, tv);
				} else
					THROWEXCEPTION("IDSLoadBalancerCfg: multiple packet selectors specified! This is not allowed.");
			} else {
				THROWEXCEPTION("Invalid selector: %s", _selector.c_str());
			}
		}
	}
	if (!selector)
		THROWEXCEPTION("IDSLoadBalancerCfg: No packet selector specified, this is compulsory");
}

IDSLoadbalancerCfg* IDSLoadbalancerCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IDSLoadbalancerCfg(elem);
}

IDSLoadbalancerCfg::~IDSLoadbalancerCfg()
{
	if (selector) delete selector;
}

IDSLoadbalancer* IDSLoadbalancerCfg::createInstance()
{
	instance = new IDSLoadbalancer(selector, updateInterval);
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
