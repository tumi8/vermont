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


#include "core/Module.h"
#include "IpfixRecordDestination.h"

/**
 * IPFIX Printer module.
 *
 * Prints received flows to stdout
 *
 * FIXME: add file support to modes TREE and LINE
 */
class IpfixPrinter : public Module, public IpfixRecordDestination, public Source<NullEmitable*>
{
	public:
		enum OutputType { TREE = 0, LINE, TABLE, NONE };

		IpfixPrinter(OutputType outputtype = TREE, string filename = "");
		~IpfixPrinter();

		virtual void onDataTemplate(IpfixDataTemplateRecord* record);
		virtual void onDataDataRecord(IpfixDataDataRecord* record);
		virtual void onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record);
		virtual void onOptionsTemplate(IpfixOptionsTemplateRecord* record);
		virtual void onOptionsRecord(IpfixOptionsRecord* record);
		virtual void onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record);
		virtual void onTemplate(IpfixTemplateRecord* record);
		virtual void onDataRecord(IpfixDataRecord* record);
		virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);

	protected:
		void* lastTemplate;
		uint32_t linesPrinted;

	private:
		OutputType outputType;
		string filename;
		FILE* fh;

		void printUint(char* buf, IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
		void printOneLineRecord(IpfixDataRecord* record);
		void printTableRecord(IpfixDataDataRecord* record);
};
void printProtocol(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);
void printFieldData(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* pattern);
void printFrontPayload(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data);

#endif
