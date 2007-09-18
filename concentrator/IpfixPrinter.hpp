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

#ifndef PRINTIPFIX_H
#define PRINTIPFIX_H

#include "IpfixParser.hpp"

#include "reconf/Module.h"
#include "IpfixRecordDestination.h"

/**
 * IPFIX Printer module.
 *
 * Prints received flows to stdout 
 */
class IpfixPrinter : public Module, public IpfixRecordDestination
{
	public:
		IpfixPrinter();
		~IpfixPrinter();

		virtual void onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		virtual void onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, 
									  uint16_t length, IpfixRecord::Data* data);
		virtual void onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		virtual void onOptionsTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
		virtual void onOptionsRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, 
									 uint16_t length, IpfixRecord::Data* data);
		virtual void onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
		virtual void onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo);
		virtual void onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, 
								  uint16_t length, IpfixRecord::Data* data);
		virtual void onTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo);

	protected:
		void* lastTemplate;
};

#endif
