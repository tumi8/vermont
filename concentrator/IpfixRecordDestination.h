/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2007 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
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


#ifndef IPFIXRECORDDESTINATION_H_
#define IPFIXRECORDDESTINATION_H_


#include "reconf/Module.h"
#include "IpfixRecord.hpp"


/**
 * this class is the base class for all vermont modules which want to receive flows
 * it calls handler functions corresponding to the received IpfixRecord type
 */
class IpfixRecordDestination : public Destination<IpfixRecord*>
{
public:
	IpfixRecordDestination();
	virtual ~IpfixRecordDestination();
	
	virtual void receive(IpfixRecord* ipfixRecord);
	
protected:
	// virtual handler functions for child classes 
	virtual void onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo);
	virtual void onOptionsTemplate(IpfixRecord::SourceID* sourceID,
			IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
	virtual void onDataTemplate(IpfixRecord::SourceID* sourceID,
			IpfixRecord::DataTemplateInfo* dataTemplateInfo);
	virtual void onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo,
			uint16_t length, IpfixRecord::Data* data);
	virtual void onOptionsRecord(IpfixRecord::SourceID* sourceID,
			IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length,
			IpfixRecord::Data* data);
	virtual void onDataDataRecord(IpfixRecord::SourceID* sourceID,
			IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length,
			IpfixRecord::Data* data);
	virtual void onTemplateDestruction(IpfixRecord::SourceID* sourceID,
			IpfixRecord::TemplateInfo* templateInfo);
	virtual void onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID,
			IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
	virtual void onDataTemplateDestruction(IpfixRecord::SourceID* sourceID,
			IpfixRecord::DataTemplateInfo* dataTemplateInfo);
};

#endif /*IPFIXRECORDDESTINATION_H_*/
