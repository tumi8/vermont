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

#include "msg.h"
#include "FlowSink.hpp"

FlowSink::FlowSink() : thread(flowSinkProcess), exitFlag(false) {
}

FlowSink::~FlowSink() {
	msg(MSG_DEBUG, "Sink: destructor called");
	terminateSink();
	msg(MSG_DEBUG, "Sink: waiting for exporter thread");
	thread.join();
	msg(MSG_DEBUG, "Sink: exporter thread joined");
}

void FlowSink::push(IpfixRecord* ipfixRecord) {
	ipfixRecords.push(ipfixRecord);
}

void* FlowSink::flowSinkProcess(void* flowSink_) {
	FlowSink* flowSink = (FlowSink*)flowSink_;
	flowSink->flowSinkProcess();
	return 0;
}

void FlowSink::flowSinkProcess()
{
	msg(MSG_INFO, "Sink: now running FlowSink thread");
	while(!exitFlag) {
		IpfixRecord* ipfixRecord;
		if (!ipfixRecords.pop(1000, &ipfixRecord)) continue;
		{
			IpfixDataRecord* rec = dynamic_cast<IpfixDataRecord*>(ipfixRecord);
			if (rec) onDataRecord(rec->sourceID, rec->templateInfo, rec->dataLength, rec->data.get());
		}
		{
			IpfixDataDataRecord* rec = dynamic_cast<IpfixDataDataRecord*>(ipfixRecord);
			if (rec) onDataDataRecord(rec->sourceID, rec->dataTemplateInfo, rec->dataLength, rec->data.get());
		}
		{
			IpfixOptionsRecord* rec = dynamic_cast<IpfixOptionsRecord*>(ipfixRecord);
			if (rec) onOptionsRecord(rec->sourceID, rec->optionsTemplateInfo, rec->dataLength, rec->data.get());
		}
		{
			IpfixTemplateRecord* rec = dynamic_cast<IpfixTemplateRecord*>(ipfixRecord);
			if (rec) onTemplate(rec->sourceID, rec->templateInfo);
		}
		{
			IpfixDataTemplateRecord* rec = dynamic_cast<IpfixDataTemplateRecord*>(ipfixRecord);
			if (rec) onDataTemplate(rec->sourceID, rec->dataTemplateInfo);
		}
		{
			IpfixOptionsTemplateRecord* rec = dynamic_cast<IpfixOptionsTemplateRecord*>(ipfixRecord);
			if (rec) onOptionsTemplate(rec->sourceID, rec->optionsTemplateInfo);
		}
		{
			IpfixTemplateDestructionRecord* rec = dynamic_cast<IpfixTemplateDestructionRecord*>(ipfixRecord);
			if (rec) onTemplateDestruction(rec->sourceID, rec->templateInfo);
		}
		{
			IpfixDataTemplateDestructionRecord* rec = dynamic_cast<IpfixDataTemplateDestructionRecord*>(ipfixRecord);
			if (rec) onDataTemplateDestruction(rec->sourceID, rec->dataTemplateInfo);
		}
		{
			IpfixOptionsTemplateDestructionRecord* rec = dynamic_cast<IpfixOptionsTemplateDestructionRecord*>(ipfixRecord);
			if (rec) onOptionsTemplateDestruction(rec->sourceID, rec->optionsTemplateInfo);
		}
		ipfixRecord->release();
		DPRINTF("SINK: free packet");
	}
}

bool FlowSink::runSink() {
	return(thread.run(this));
}

bool FlowSink::terminateSink() {
	exitFlag = true;
	return true;
}

