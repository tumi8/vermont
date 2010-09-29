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



#include "IpfixCollector.hpp"

#include "IpfixReceiver.hpp"
#include "IpfixParser.hpp"
#include "common/msg.h"



/**
 * Creates a new IpfixCollector.
 * Call @c startIpfixCollector() to start receiving and processing messages.
 */
IpfixCollector::IpfixCollector(IpfixReceiver* receiver)
	: ipfixReceiver(receiver),
	  statSentRecords(0)
{
	ipfixPacketProcessor = new IpfixParser(this);
	receiver->setVModule(this);
	
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
	// to make sure that exitFlag is set and performShutdown() is called
	this->shutdown(false);
	delete ipfixReceiver;
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
	connected.shutdown();
}

void IpfixCollector::postReconfigration()
{ 
	ipfixPacketProcessor->postReconfiguration();
}

void IpfixCollector::onReconfiguration1()
{
	ipfixPacketProcessor->onReconfiguration1();
}

void IpfixCollector::onReconfiguration2()
{
	ipfixPacketProcessor->onReconfiguration2();
}

/**
 * just delegates call to Source::send and collects statistics
 * (needed for interface IpfixRecordSender
 */
bool IpfixCollector::send(IpfixRecord* ipfixRecord)
{
	// do not send anything any more, if module is to be stopped
	if (exitFlag) return false;
	
	statSentRecords++;
	return Source<IpfixRecord*>::send(ipfixRecord);	
}

string IpfixCollector::getStatisticsXML(double interval)
{
	char buf[50];
	snprintf(buf, ARRAY_SIZE(buf), "<sentRecords>%llu</sentRecords>", (long long unsigned)statSentRecords);
	return buf;
}

/* Set template lifetime of IpfixParser
 */
void IpfixCollector::setTemplateLifetime(uint16_t time)
{
	if(ipfixPacketProcessor && dynamic_cast<IpfixParser*>(ipfixPacketProcessor))
		dynamic_cast<IpfixParser*>(ipfixPacketProcessor)->setTemplateLifetime(time);
	else
		msg(MSG_ERROR, "IpfixCollector: Cannot set template lifetime, ipfixPacketProcessor is NULL");
}
