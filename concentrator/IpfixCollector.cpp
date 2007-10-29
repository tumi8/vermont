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



#include "IpfixCollector.hpp"

#include "IpfixReceiver.hpp"
#include "IpfixParser.hpp"
#include "common/msg.h"



/**
 * Creates a new IpfixCollector.
 * Call @c startIpfixCollector() to start receiving and processing messages.
 */
IpfixCollector::IpfixCollector(IpfixReceiver* receiver)
	: ipfixReceiver(receiver)
{
	ipfixPacketProcessor = new IpfixParser(this);
	
	// wire ipfixReceiver with ipfixPacketProcessor
	list<IpfixPacketProcessor*> pplist;
	pplist.push_back(ipfixPacketProcessor);
	ipfixReceiver->setPacketProcessors(pplist);
}

/**
 * Frees memory used by a IpfixCollector.
 */
IpfixCollector::~IpfixCollector() 
{
	delete ipfixPacketProcessor;
}

/**
 * Starts receiving and processing messages by starting the
 * ipfixReceiver
 */
void IpfixCollector::performStart() 
{
	ipfixReceiver->performStart();
}

/**
 * Stops processing messages.
 */
void IpfixCollector::performShutdown() 
{
	ipfixReceiver->performShutdown();
}


/**
 * just delegates call to Source::send
 * (needed for interface IpfixRecordSender
 */
bool IpfixCollector::send(IpfixRecord* ipfixRecord)
{
	return Source<IpfixRecord*>::send(ipfixRecord);	
}
