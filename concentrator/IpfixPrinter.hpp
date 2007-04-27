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

/**
 * IPFIX Printer module.
 *
 * Prints received flows to stdout 
 */
class IpfixPrinter : public FlowSink {
	public:
		IpfixPrinter();
		~IpfixPrinter();

		void start();
		void stop();

		int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data);
		int onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);

		int onOptionsTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);
		int onOptionsRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, IpfixRecord::Data* data);
		int onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo);

		int onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo);
		int onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data);
		int onTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo);

	protected:
		void* lastTemplate;
};

#endif
