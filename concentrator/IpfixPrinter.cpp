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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "IpfixPrinter.hpp"


/**
 * Creates a new IpfixPrinter. Do not forget to call @c startIpfixPrinter() to begin printing
 * @return handle to use when calling @c destroyIpfixPrinter()
 */
IpfixPrinter::IpfixPrinter() 
{
	lastTemplate = 0;	
}

/**
 * Frees memory used by an IpfixPrinter
 */
IpfixPrinter::~IpfixPrinter() 
{
}


/**
 * Prints a Template
 * @param sourceID SourceID of the exporting process
 * @param templateInfo Pointer to a structure defining the Template used
 */
void IpfixPrinter::onTemplate(IpfixTemplateRecord* record) 
{
	printf("\n-+--- Template (id=%u)\n", record->templateInfo->templateId);
	printf(" `---\n\n");
	record->removeReference();
}

/**
 * Prints a Template that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onTemplateDestruction(IpfixTemplateDestructionRecord* record) 
{
	printf("Destroyed a Template (id=%u)\n", record->templateInfo->templateId);
	record->removeReference();
}


void IpfixPrinter::printUint(char* buf, IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data) {
	switch (type.length) {
	case 1:
		sprintf(buf, "%hhu",*(uint8_t*)data);
		return;
	case 2:
		sprintf(buf, "%hu",ntohs(*(uint16_t*)data));
		return;
	case 4:
		sprintf(buf, "%u",ntohl(*(uint32_t*)data));
		return;
	case 8:
		sprintf(buf, "%Lu",ntohll(*(uint64_t*)data));
		return;
	default:
		msg(MSG_ERROR, "Uint with length %d unparseable", type.length);
		return;
	}
}

/**
 * Prints a DataRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixPrinter::onDataRecord(IpfixDataRecord* record)
{
	int i;

	printf("\n-+--- DataRecord (Template id=%u)\n", record->templateInfo->templateId);
	printf(" `- variable data\n");
	for (i = 0; i < record->templateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(record->templateInfo->fieldInfo[i].type, 
				(record->data + record->templateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
}

/**
 * Prints a OptionsTemplate
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onOptionsTemplate(IpfixOptionsTemplateRecord* record) 
{
	printf("\n-+--- OptionsTemplate (id=%u)\n", record->optionsTemplateInfo->templateId);
	printf(" `---\n\n");
	record->removeReference();
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onOptionsTemplateDestruction(IpfixOptionsTemplateDestructionRecord* record) 
{
	printf("Destroyed an OptionsTemplate (id=%u)\n", record->optionsTemplateInfo->templateId);
	record->removeReference();
}

/**
 * Prints an OptionsRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixPrinter::onOptionsRecord(IpfixOptionsRecord* record)
{
	printf("\n-+--- OptionsDataRecord (Template id=%u)\n", record->optionsTemplateInfo->templateId);
	printf(" `---\n\n");
	record->removeReference();
}

/**
 * Prints a DataTemplate
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onDataTemplate(IpfixDataTemplateRecord* record)
{
	int i;

	printf("\n-+--- DataTemplate (id=%u)\n", record->dataTemplateInfo->templateId);
	printf(" `- fixed data\n");
	for (i = 0; i < record->dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(record->dataTemplateInfo->dataInfo[i].type, 
				(record->dataTemplateInfo->data + record->dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
	record->removeReference();
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onDataTemplateDestruction(IpfixDataTemplateDestructionRecord* record) 
{
	printf("Destroyed a DataTemplate (id=%u)\n", record->dataTemplateInfo->templateId);
	record->removeReference();
}

/**
 * Prints a DataDataRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixPrinter::onDataDataRecord(IpfixDataDataRecord* record)
{
	boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo = record->dataTemplateInfo;
	int i;

	printf("\n-+--- DataDataRecord (Template id=%u)\n", dataTemplateInfo->templateId);
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, 
				(dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `- variable data\n");
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->fieldInfo[i].type, 
				(record->data + dataTemplateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
	record->removeReference();
}

