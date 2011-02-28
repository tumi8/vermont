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

#include "TemplateBuffer.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

void TemplateBuffer::BufferedTemplate::onPreDestroy(IpfixParser* ipfixParser) {
	IpfixTemplateDestructionRecord* ipfixRecord = ipfixParser->templateDestructionRecordIM.getNewInstance();
	ipfixRecord->sourceID = sourceID;
	ipfixRecord->templateInfo = templateInfo;
	ipfixParser->ipfixRecordSender->send(ipfixRecord);
}

bool TemplateBuffer::BufferedTemplate::isExpired() {
	return expires && expires < time(NULL);
}

/**
 * Returns a TemplateInfo or NULL
 */
TemplateBuffer::BufferedTemplate* TemplateBuffer::getBufferedTemplate(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::TemplateId templateId) {
	TemplateBuffer::BufferedTemplate* bt = head;

#ifdef DEBUG
	DPRINTF("ALL TEMPLATES ---------------------------");
	while (bt != 0) {
		DPRINTF("bt->sourceID odid %lu exporter port %u collector port %u exporter ip %u.%u.%u.%u len %u prot %u ptr: %lu size: %lu expires in %d sec", bt->sourceID.get()->observationDomainId, bt->sourceID.get()->exporterPort, bt->sourceID.get()->receiverPort, bt->sourceID.get()->exporterAddress.ip[0], bt->sourceID.get()->exporterAddress.ip[1], bt->sourceID.get()->exporterAddress.ip[2], bt->sourceID.get()->exporterAddress.ip[3], bt->sourceID.get()->exporterAddress.len, bt->sourceID.get()->protocol, bt->sourceID.get(), sizeof(IpfixRecord::SourceID), bt->expires - time(NULL));
		
		bt = bt->next;
	}
	DPRINTF("END ALL TEMPLATES --------------------------");
	
	bt = head;
	DPRINTF("Searching for : sourceID %lu %u %u %u.%u.%u.%u  %u %u", sourceId.get()->observationDomainId, sourceId.get()->exporterPort, sourceId.get()->receiverPort, sourceId.get()->exporterAddress.ip[0], sourceId.get()->exporterAddress.ip[1], sourceId.get()->exporterAddress.ip[2], sourceId.get()->exporterAddress.ip[3], sourceId.get()->exporterAddress.len, sourceId.get()->protocol);
#endif
	
	while (bt != 0) {
		if ((*(bt->sourceID.get()) == *(sourceId.get())) && (bt->templateInfo->templateId == templateId)){
			if (bt->isExpired()) {
				DPRINTF("Template found but expired.");
				cleanUpExpiredTemplates();
				return 0;
			}
			DPRINTF("Template found.");
			return bt;
		}
		bt = bt->next;
	}
	DPRINTF("getBufferedTemplate not found!!!");
	return 0;
}

/**
 * Saves a TemplateInfo, IpfixRecord::OptionsTemplateInfo, IpfixRecord::DataTemplateInfo overwriting existing Templates
 */
void TemplateBuffer::bufferTemplate(TemplateBuffer::BufferedTemplate* bt) {
	cleanUpExpiredTemplates();
	destroyBufferedTemplate(bt->sourceID, bt->templateInfo->templateId);
	bt->next = head;
	bt->expires = 0;
	head = bt;
}

/**
 * Saves a TemplateInfo, IpfixRecord::OptionsTemplateInfo, IpfixRecord::DataTemplateInfo overwriting existing Templates
 */
void TemplateBuffer::cleanUpExpiredTemplates() {
	TemplateBuffer::BufferedTemplate* predecessor = 0;
	TemplateBuffer::BufferedTemplate* bt = head;
	while (bt != 0) {
		if (bt->isExpired()) {
			DPRINTF("Cleaning up expired template with id %d",bt->templateInfo->templateId);
			bt->onPreDestroy(ipfixParser);
			if (predecessor)
				predecessor->next = bt->next;
			else
				head = bt->next;
			TemplateBuffer::BufferedTemplate* toBeFreed = bt;
			bt = bt->next;
			delete toBeFreed;
			/* leave predecessor unchanged. */
		} else {
			predecessor = bt;
			bt = bt->next;
		}
	}
}

/**
 * Frees memory, marks Template unused.
 */
void TemplateBuffer::destroyBufferedTemplate(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::TemplateId templateId, bool all) 
{
	TemplateBuffer::BufferedTemplate* predecessor = 0;
	TemplateBuffer::BufferedTemplate* bt = head;
	bool found = false;
	while (bt != 0) {
		/* templateId == setID means that all templates of this set type shall be removed for given sourceID */
		/* all == true means that all templates of given sourceID shall be removed */
		if (((*(bt->sourceID.get()) == *(sourceId.get())) && ((bt->templateInfo->templateId == templateId) || (bt->templateInfo->setId == templateId)))
				|| (all && sourceId->equalIgnoringODID(*(bt->sourceID.get())))) {
			found = true;
			DPRINTF("Destroying template with id %u", bt->templateInfo->templateId);
			if (predecessor != 0) {
				predecessor->next = bt->next;
			} else {
				head = bt->next;
			}
			/* Invoke all registered callback functions */
			bt->onPreDestroy(ipfixParser);
			TemplateBuffer::BufferedTemplate* toBeFreed = bt;
			bt = bt->next;
			delete toBeFreed;
		} else {
			predecessor = bt;
			bt = bt->next;
		}
	}
	if (!found && !all) {
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
		head = bt->next;
		delete bt;
	}
}

/**
 * returns first template in list
 */
TemplateBuffer::BufferedTemplate* TemplateBuffer::getFirstBufferedTemplate()
{
	return head;
}

