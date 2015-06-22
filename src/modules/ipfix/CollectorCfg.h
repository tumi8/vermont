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

#include <modules/ipfix/IpfixCollectorCfg.h>
#include <modules/ipfix/IpfixReceiverUdpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverSctpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverDtlsUdpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverDtlsSctpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverTcpIpV4.hpp>
#include <modules/ipfix/IpfixReceiverFile.hpp>

#include <common/ipfixlolib/ipfixlolib.h>


#include <string>
#include <set>

/**
 * This class holds the <collector> ... </collector> and the <listener> ...
 * </listener> information of the config
 */
class CollectorCfg
{
public:
	std::string getName() { return "collector"; }

	CollectorCfg(XMLElement* elem)
		: protocol(UDP), port(0), mtu(0)
	{
		uint16_t defaultPort = 4739;
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
				if (prot=="17" || prot=="UDP") {
					protocol = UDP;
					defaultPort = 4739;
				} else if (prot=="132" || prot=="SCTP") {
					protocol = SCTP;
					defaultPort = 4739;
				} else if (prot=="DTLS_OVER_UDP") {
					protocol = DTLS_OVER_UDP;
					defaultPort = 4740;
				} else if (prot=="DTLS_OVER_SCTP") {
					protocol = DTLS_OVER_SCTP;
					defaultPort = 4740;
				} else if (prot=="TCP") {
					protocol = TCP;
					defaultPort = 4740;
				} else
					THROWEXCEPTION("Invalid configuration parameter for transportProtocol (%s)", prot.c_str());
			} else if (e->matches("port")) {
				port = (uint16_t)atoi(e->getContent().c_str());
				if (port == 0)
					THROWEXCEPTION("Invalid configuration parameter for port (%u)", port);
			} else if (e->matches("mtu")) {
				mtu = (uint16_t)atoi(e->getContent().c_str());
			} else if (e->matches("peerFqdn")) {
				string strdnsname(e->getFirstText());
				transform(strdnsname.begin(),strdnsname.end(),strdnsname.begin(),
						::tolower);
				peerFqdns.insert(strdnsname);
			} else if (e->matches("buffer")) {
				buffer = (uint32_t)atoi(e->getContent().c_str());
			} else {
				msg(MSG_FATAL, "Unknown collector config statement %s", e->getName().c_str());
				continue;
			}
		}
		if (port==0) port = defaultPort;

		/*
		 * Sort the lists here, so that in equalTo() we can return True even if
		 * the order does not match, since what we care about is the content
		 */
		std::sort(authorizedHosts.begin(), authorizedHosts.end());
	}

	IpfixReceiver* createIpfixReceiver(
			const std::string &certificateChainFile,
			const std::string &privateKeyFile,
			const std::string &caFile,
			const std::string &caPath) {
		IpfixReceiver* ipfixReceiver;
		if (protocol == SCTP)
			ipfixReceiver = new IpfixReceiverSctpIpV4(port, ipAddress, buffer);
		else if (protocol == DTLS_OVER_UDP)
			ipfixReceiver = new IpfixReceiverDtlsUdpIpV4(port,
				ipAddress, certificateChainFile,
				privateKeyFile, caFile, caPath, peerFqdns, buffer);
		else if (protocol == DTLS_OVER_SCTP)
			ipfixReceiver = new IpfixReceiverDtlsSctpIpV4(port,
				ipAddress, certificateChainFile,
				privateKeyFile, caFile, caPath, peerFqdns, buffer);
		else if (protocol == TCP)
			ipfixReceiver = new IpfixReceiverTcpIpV4(port, ipAddress, buffer);
		else
			ipfixReceiver = new IpfixReceiverUdpIpV4(port, ipAddress, buffer);

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
			(mtu == other->mtu) &&
			(peerFqdns == other->peerFqdns) &&
			(buffer == other->buffer) &&
			(authorizedHosts == other->authorizedHosts)) {
			return true;
		}

		return false;
	}

	std::set<std::string> getPeerFqdns() { return peerFqdns; }
	std::string getIpAddress() { return ipAddress; }
	//unsigned getIpAddressType() { return ipAddressType; }
	ipfix_transport_protocol getProtocol() { return protocol; }
	uint16_t getPort() { return port; }
	uint16_t getMtu() { return mtu; }

private:
	std::string ipAddress;
	std::vector<std::string> authorizedHosts;
	ipfix_transport_protocol protocol;
	uint16_t port;
	uint16_t mtu;
	uint32_t buffer;
	std::set<std::string> peerFqdns;
};

#endif /*COLLECTORCFG_H_*/
