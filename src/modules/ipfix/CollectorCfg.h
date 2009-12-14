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

#ifndef COLLECTORCFG_H_
#define COLLECTORCFG_H_

#include "core/Cfg.h"

#include <modules/ipfix/IpfixReceiverUdpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverSctpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverFile.hpp>


#include <string>

/**
 * This class holds the <collector> ... </collector> information of the config
 */
class CollectorCfg
{
public:
	std::string getName() { return "collector"; }

	CollectorCfg(XMLElement* elem)
		: protocol(0), port(4739)
	{
		if (!elem)
			return;

		XMLNode::XMLSet<XMLElement*> set = elem->getElementChildren();
		for (XMLNode::XMLSet<XMLElement*>::iterator it = set.begin(); it != set.end(); it++) {
			XMLElement* e = *it;

			if (e->matches("ipAddress")) {
				ipAddress = e->getContent();
			} else if (e->matches("authorizedHost")) {
				authorizedHosts.push_back(e->getContent());
			} else if (e->matches("transportProtocol")) {
				std::string prot = e->getContent();
				if (prot=="17" || prot=="UDP")
					protocol = 17;
				else if (prot=="132" || prot=="SCTP")
					protocol = 132;
				else
					THROWEXCEPTION("Invalid configuration parameter for transportProtocol (%s)", prot.c_str());
			} else if (e->matches("port")) {
				port = (uint16_t)atoi(e->getContent().c_str());
				if (port == 0)
					THROWEXCEPTION("Invalid configuration parameter for port (%u)", port);
			} else {
				msg(MSG_FATAL, "Unknown collector config statement %s\n", e->getName().c_str());
				continue;
			}
		}
	}

	IpfixReceiver* createIpfixReceiver() {
		IpfixReceiver* ipfixReceiver;
		if (protocol == 132)
			ipfixReceiver = new IpfixReceiverSctpIpV4(port, ipAddress);
		else
			ipfixReceiver = new IpfixReceiverUdpIpV4(port, ipAddress);

		if (!ipfixReceiver) {
			THROWEXCEPTION("Could not create IpfixReceiver");
		}

		for (std::vector<std::string>::iterator i = authorizedHosts.begin(); i != authorizedHosts.end(); i++) {
			if (ipfixReceiver->addAuthorizedHost((*i).c_str()) != 0) 
				msg(MSG_ERROR, "CollectorCfg: Invalid authorized host %s", (*i).c_str());
		}
		return ipfixReceiver;
	}

	bool equalTo(CollectorCfg* other)
	{
		if ((ipAddress == other->ipAddress) &&
			//(ipAddressType == other->ipAddressType) &&
			(protocol == other->protocol) &&
			(port == other->port) &&
			(authorizedHosts.size() == other->authorizedHosts.size())) {
			for (uint16_t i = 0; i < authorizedHosts.size(); i++)
				if (authorizedHosts[i] != other->authorizedHosts[i])
					return false;	
			return true;
		}

		return false;
	}

	std::string getIpAddress() { return ipAddress; }
	//unsigned getIpAddressType() { return ipAddressType; }
	uint16_t getProtocol() { return protocol; }
	uint16_t getPort() { return port; }

private:
	std::string ipAddress;
	std::vector<std::string> authorizedHosts;
	uint16_t protocol;
	uint16_t port;
};

#endif /*COLLECTORCFG_H_*/
