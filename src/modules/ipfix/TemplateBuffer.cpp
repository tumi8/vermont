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

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TemplateBuffer.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

/**
 * Returns a IpfixRecord::TemplateInfo, IpfixRecord::OptionsTemplateInfo, IpfixRecord::DataTemplateInfo or NULL
 */
TemplateBuffer::BufferedTemplate* TemplateBuffer::getBufferedTemplate(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateID templateId) {
	time_t now = time(0);
	TemplateBuffer::BufferedTemplate* bt = head;

#ifdef DEBUG
	DPRINTF("ALL TEMPLATES ---------------------------");
	while (bt != 0) {
		DPRINTF("bt->sourceID %lu %u %u %u.%u.%u.%u %u %u ptr: %lu size: %lu", bt->sourceID.get()->observationDomainId, bt->sourceID.get()->exporterPort, bt->sourceID.get()->receiverPort, bt->sourceID.get()->exporterAddress.ip[0], bt->sourceID.get()->exporterAddress.ip[1], bt->sourceID.get()->exporterAddress.ip[2], bt->sourceID.get()->exporterAddress.ip[3], bt->sourceID.get()->exporterAddress.len, bt->sourceID.get()->protocol, bt->sourceID.get(), sizeof(IpfixRecord::SourceID));
		
		bt = (TemplateBuffer::BufferedTemplate*)bt->next;
	}
	DPRINTF("END ALL TEMPLATES --------------------------");
	
	bt = head;
	DPRINTF("Searching for : sourceID %lu %u %u %u.%u.%u.%u  %u %u", sourceId.get()->observationDomainId, sourceId.get()->exporterPort, sourceId.get()->receiverPort, sourceId.get()->exporterAddress.ip[0], sourceId.get()->exporterAddress.ip[1], sourceId.get()->exporterAddress.ip[2], sourceId.get()->exporterAddress.ip[3], sourceId.get()->exporterAddress.len, sourceId.get()->protocol);
#endif
	
	while (bt != 0) {
		if ((*(bt->sourceID.get()) == *(sourceId.get())) && (bt->templateID == templateId)){
			if ((bt->expires) && (bt->expires < now)) {
				destroyBufferedTemplate(sourceId, templateId);
				return 0;
			}
			return bt;
		}
		bt = (TemplateBuffer::BufferedTemplate*)bt->next;
	}
	DPRINTF("getBufferedTemplate not found!!!");
	return 0;
}

/**
 * Saves a IpfixRecord::TemplateInfo, IpfixRecord::OptionsTemplateInfo, IpfixRecord::DataTemplateInfo overwriting existing Templates
 */
void TemplateBuffer::bufferTemplate(TemplateBuffer::BufferedTemplate* bt) {
	destroyBufferedTemplate(bt->sourceID, bt->templateID);
	bt->next = head;
	bt->expires = 0;
	head = bt;
}

/**
 * Frees memory, marks Template unused.
 */
void TemplateBuffer::destroyBufferedTemplate(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateID templateId, bool all) 
{
	TemplateBuffer::BufferedTemplate* predecessor = 0;
	TemplateBuffer::BufferedTemplate* bt = head;
	bool found = false;
	while (bt != 0) {
		/* templateId == setID means that all templates of this set type shall be removed for given sourceID */
		/* all == true means that all templates of given sourceID shall be removed */
		if (((*(bt->sourceID.get()) == *(sourceId.get())) && ((bt->templateID == templateId) || (bt->setID == templateId)) 
				|| (all && sourceId->equalIgnoringODID(*(bt->sourceID.get()))))) {
			found = true;
			if (predecessor != 0) {
				predecessor->next = bt->next;
			} else {
				head = (TemplateBuffer::BufferedTemplate*)bt->next;
			}
			if (bt->setID == IPFIX_SetId_Template) {
				/* Invoke all registered callback functions */
				IpfixTemplateDestructionRecord* ipfixRecord = ipfixParser->templateDestructionRecordIM.getNewInstance();
				ipfixRecord->sourceID = bt->sourceID;

				ipfixRecord->templateInfo =  boost::shared_ptr<IpfixRecord::DataTemplateInfo>(new IpfixRecord::DataTemplateInfo(*bt->templateInfo.get()));
				ipfixParser->ipfixRecordSender->send(ipfixRecord);
			} else
#ifdef SUPPORT_NETFLOWV9
			if (bt->setID == NetflowV9_SetId_Template) {
				/* Invoke all registered callback functions */
				IpfixTemplateDestructionRecord* ipfixRecord = ipfixParser->templateDestructionRecordIM.getNewInstance();
				ipfixRecord->sourceID = bt->sourceID;
				ipfixRecord->templateInfo =  boost::shared_ptr<IpfixRecord::DataTemplateInfo>(new IpfixRecord::DataTemplateInfo(*bt->templateInfo.get()));
				ipfixParser->ipfixRecordSender->send(ipfixRecord);
			} else
#endif
			if (bt->setID == IPFIX_SetId_OptionsTemplate) {
				/* Invoke all registered callback functions */
				IpfixOptionsTemplateDestructionRecord* ipfixRecord = ipfixParser->optionsTemplateDestructionRecordIM.getNewInstance();
				ipfixRecord->sourceID = bt->sourceID;
				ipfixRecord->optionsTemplateInfo = bt->optionsTemplateInfo;
				ipfixParser->ipfixRecordSender->send(ipfixRecord);
			} else if (bt->setID == IPFIX_SetId_DataTemplate) {
				/* Invoke all registered callback functions */
				IpfixTemplateDestructionRecord* ipfixRecord = ipfixParser->templateDestructionRecordIM.getNewInstance();
				ipfixRecord->sourceID = bt->sourceID;
				ipfixRecord->templateInfo = bt->dataTemplateInfo;
				ipfixParser->ipfixRecordSender->send(ipfixRecord);

			} else {
				msg(MSG_FATAL, "Unknown template type requested to be freed: %d", bt->setID);
			}
			TemplateBuffer::BufferedTemplate* toBeFreed = bt;
			bt = (TemplateBuffer::BufferedTemplate*)bt->next;
			delete toBeFreed;
		} else {
			predecessor = bt;
			bt = (TemplateBuffer::BufferedTemplate*)bt->next;
		}
	}
	if (!found) {
		DPRINTF("Destroy template - no matching template found (id=%u)", templateId);
	}
		
}

/**
 * initializes the buffer
 */
TemplateBuffer::TemplateBuffer(IpfixParser* parentIpfixParser) {
	head = 0;
	ipfixParser = parentIpfixParser;
}

/**
 * Destroys all buffered templates
 */
TemplateBuffer::~TemplateBuffer() {
	while (head != 0) {
		TemplateBuffer::BufferedTemplate* bt = head;
		TemplateBuffer::BufferedTemplate* bt2 = (TemplateBuffer::BufferedTemplate*)bt->next;
		destroyBufferedTemplate(bt->sourceID, bt->templateID);
		head = bt2;
	}
}

/**
 * returns first template in list
 */
TemplateBuffer::BufferedTemplate* TemplateBuffer::getFirstBufferedTemplate()
{
	return head;
}

