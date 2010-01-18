/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2009 Vermont Project
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

#include "IpfixCollectorCfg.h"

IpfixCollectorCfg::IpfixCollectorCfg(XMLElement* elem)
	: CfgHelper<IpfixCollector, IpfixCollectorCfg>(elem, "ipfixCollector"),
	listener(NULL),
	ipfixCollector(NULL)
{

	if (!elem)
		return;

	msg(MSG_INFO, "IpfixCollectorCfg: Start reading ipfixCollector section");
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("listener")) {
			listener = new CollectorCfg(e);
		} else if (e->matches("next")) { // ignore next
		} else {
			msg(MSG_FATAL, "Unkown observer config statement %s\n", e->getName().c_str());
			continue;
		}
	}

	udpTemplateLifetime = getInt("udpTemplateLifetime", -1);

	if (listener == NULL)
		THROWEXCEPTION("collectingProcess has to listen on one address!");

	msg(MSG_INFO, "IpfixCollectorCfg: Successfully parsed collectingProcess section");
}

IpfixCollectorCfg::~IpfixCollectorCfg()
{
}

IpfixCollectorCfg* IpfixCollectorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixCollectorCfg(elem);
}

IpfixCollector* IpfixCollectorCfg::createInstance()
{
	instance = new IpfixCollector(listener->createIpfixReceiver());
	if(udpTemplateLifetime>=0)
		instance->setTemplateLifetime((uint16_t)udpTemplateLifetime);
	return instance;
}

bool IpfixCollectorCfg::deriveFrom(IpfixCollectorCfg* old)
{
	/*
	 * The template handling in IpfixCollector/IpfixParser must be reworked before implementing
	 * this function, because the templates must all be invalidated on preReConfigure1().
	 * Invalid templates must be removed in preReconfigure2() and the new templates
	 * must be transmited on preConnect()
	 */
	return false;   // FIXME: implement it, to gain performance increase in reconnect
}
