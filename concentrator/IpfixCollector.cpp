/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

// FIXME: Basic support for NetflowV9 packets, templates and flow records is provided. Will break when fed field types with type ID >= 0x8000.

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

/* for ntohll et al */
#include "ipfixlolib/ipfixlolib.h"

#include "IpfixReceiver.hpp"
#include "TemplateBuffer.hpp"
#include "ipfix.hpp"

#include "common/msg.h"

#include "IpfixCollector.hpp"

/**
 * Adds a PacketProcessor to the list of PacketProcessors
 * @param ipfixCollector Collector to assign the PacketProcessor to
 * @param packetProcessor handle of packetProcessor
 */
void IpfixCollector::addIpfixPacketProcessor(IpfixPacketProcessor* packetProcessor) {
	packetProcessors.push_back(packetProcessor);

	for (std::list<IpfixReceiver*>::iterator i = ipfixReceivers.begin(); i != ipfixReceivers.end(); i++) {
		(*i)->setPacketProcessors(packetProcessors);
	}
}


/**
 * Creates a new IpfixCollector.
 * Call @c startIpfixCollector() to start receiving and processing messages.
 */
IpfixCollector::IpfixCollector() {
}

/**
 * Frees memory used by a IpfixCollector.
 * @param ipfixCollector Handle returned by @c createIpfixCollector()
 */
IpfixCollector::~IpfixCollector() {
	for (std::list<IpfixReceiver*>::iterator i = ipfixReceivers.begin(); i != ipfixReceivers.end(); i++) {
		//FIXME: who should delete those?
		//delete (*i);
	}
	for (std::list<IpfixPacketProcessor*>::iterator i = packetProcessors.begin(); i != packetProcessors.end(); i++) {
		//FIXME: who should delete those?
		//delete (*i);
	}
}

/**
 * Starts receiving and processing messages.
 * All sockets prepared by calls to createIpfixCollector() will start
 * receiving messages until stopIpfixCollector() is called.
 * @return 0 on success, non-zero on error
 */
int IpfixCollector::start() {
	int err = 0;
	for (std::list<IpfixReceiver*>::iterator i = ipfixReceivers.begin(); i != ipfixReceivers.end(); i++) {
		err += (*i)->start(); 
	}
	return err;
}

/**
 * Stops processing messages.
 * No more messages will be processed until the next startIpfixCollector() call.
 * @return 0 on success, non-zero on error
 */
int IpfixCollector::stop() {
	int err = 0;
	for (std::list<IpfixReceiver*>::iterator i = ipfixReceivers.begin(); i != ipfixReceivers.end(); i++) {
		err += (*i)->stop(); 
	}
	return err;
}

/**
 * Adds a IpfixReceiver to the list of IpfixReceivers
 * @param ipfixCollector Collector to assign the IpfixReceiver to
 * @param ipfixReceiver handle of ipfixReceiver
 */
void IpfixCollector::addIpfixReceiver(IpfixReceiver* ipfixReceiver) {
	ipfixReceivers.push_back(ipfixReceiver);
}

