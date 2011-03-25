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
#include "CollectorCfg.h"
#include <modules/ipfix/IpfixReceiverUdpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverDtlsUdpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverSctpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverDtlsSctpIpV4.hpp>

IpfixCollectorCfg::IpfixCollectorCfg(XMLElement* elem)
	: CfgHelper<IpfixCollector, IpfixCollectorCfg>(elem, "ipfixCollector"),
	listener(NULL),
	ipfixCollector(NULL)
{
	if (!elem)
		return;

	msg(MSG_INFO, "IpfixCollectorCfg: Start reading ipfixCollector section");
	udpTemplateLifetime = getInt("udpTemplateLifetime", -1);

	// Config for DTLS
	certificateChainFile = getOptional("cert");
	privateKeyFile = getOptional("key");
	caFile = getOptional("CAfile");
	caPath = getOptional("CApath");
	// observationDomainId = getInt("observationDomainId", 0);
	
	XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
	for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin();
	     it != set.end();
	     it++) {
		XMLElement* e = *it;

		if (e->matches("listener")) {
			if (listener)
				THROWEXCEPTION("listener already set. There can only be one <listener> Element per Collector.");
			listener = new CollectorCfg(e);
			if (listener->getMtu() != 0) {
				delete listener;
				THROWEXCEPTION("You can not set the MTU for a listener.");
			}
		} else if (e->matches("udpTemplateLifetime")) { // already done
		} else if (e->matches("next")) { // ignore next
		} else if (e->matches("cert") || e->matches("key") ||
				e->matches("CAfile") || e->matches("CApath")) {
			// already done!
		} else {
			msg(MSG_FATAL, "Unkown collector config statement %s", e->getName().c_str());
			continue;
		}
	}

	if (listener == NULL)
		THROWEXCEPTION("collectingProcess has to listen on one address!");

	if (listener->getProtocol() != UDP &&
			listener->getProtocol() != TCP && 
			listener->getProtocol() != SCTP &&
			listener->getProtocol() != DTLS_OVER_UDP &&
			listener->getProtocol() != DTLS_OVER_SCTP)
		THROWEXCEPTION("collectingProcess can handle only UDP, TCP, or SCTP!");
	
	msg(MSG_INFO, "IpfixCollectorCfg: Successfully parsed collectingProcess section");
}

IpfixCollectorCfg::~IpfixCollectorCfg()
{
	// FIXME: Shouldn't we delete listener here?
}

IpfixCollectorCfg* IpfixCollectorCfg::create(XMLElement* elem)
{
	assert(elem);
	assert(elem->getName() == getName());
	return new IpfixCollectorCfg(elem);
}

IpfixCollector* IpfixCollectorCfg::createInstance()
{
	instance = new IpfixCollector(listener->createIpfixReceiver(certificateChainFile, privateKeyFile, caFile, caPath));
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
