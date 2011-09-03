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

class PrintHelpers
{
	public:
		PrintHelpers() : fh(stdout) {}

		void printFieldData(InformationElement::IeInfo type, IpfixRecord::Data* pattern);
		void printIPv4(uint32_t data);
		void printIPv4(InformationElement::IeInfo type, IpfixRecord::Data* data);
		void printPort(InformationElement::IeInfo type, IpfixRecord::Data* data);
		void printProtocol(uint8_t data);
		void printProtocol(InformationElement::IeInfo type, IpfixRecord::Data* data);
		void printUint(InformationElement::IeInfo type, IpfixRecord::Data* data);
		void printUint(char* buf, InformationElement::IeInfo type, IpfixRecord::Data* data);
		void printLocaltime(InformationElement::IeInfo type, IpfixRecord::Data* data);
		void printFrontPayload(InformationElement::IeInfo type, IpfixRecord::Data* data);

	protected:
		FILE* fh;
};

/**
 * IPFIX Printer module.
 *
 * Prints received flows to stdout or file
 */
class IpfixPrinter : public Module, public IpfixRecordDestination, public Source<NullEmitable*>, private PrintHelpers
{
	public:
		enum OutputType { TREE = 0, LINE, TABLE, NONE };

		IpfixPrinter(OutputType outputtype = TREE, string filename = "");
		~IpfixPrinter();

		virtual void onDataRecord(IpfixDataRecord* record);
		virtual void onTemplate(IpfixTemplateRecord* record);
		virtual void onTemplateDestruction(IpfixTemplateDestructionRecord* record);

	protected:
		void* lastTemplate;
		uint32_t linesPrinted;

	private:
		OutputType outputType;
		string filename;

		void printOneLineRecord(IpfixDataRecord* record);
		void printTreeRecord(IpfixDataRecord* record);
		void printTableRecord(IpfixDataRecord* record);

		void printStructuredData(IpfixRecord::Data *data, const TemplateInfo::FieldInfo &fieldInfo, uint16_t indentLevel = 1);
};

#endif
