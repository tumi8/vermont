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
void IpfixPrinter::onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) 
{
	printf("\n-+--- Template (id=%u)\n", templateInfo->templateId);
	printf(" `---\n\n");
}

/**
 * Prints a Template that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onTemplateDestruction(IpfixRecord::SourceID* sourceID, 
										 IpfixRecord::TemplateInfo* templateInfo) 
{
	printf("Destroyed a Template (id=%u)\n", templateInfo->templateId);

}

/**
 * Prints a DataRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixPrinter::onDataRecord(IpfixRecord::SourceID* sourceID, 
								IpfixRecord::TemplateInfo* templateInfo, 
								uint16_t length, IpfixRecord::Data* data) 
{
	int i;

	printf("\n-+--- DataRecord (Template id=%u)\n", templateInfo->templateId);
	printf(" `- variable data\n");
	for (i = 0; i < templateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(templateInfo->fieldInfo[i].type, (data + templateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
}

/**
 * Prints a OptionsTemplate
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onOptionsTemplate(IpfixRecord::SourceID* sourceID, 
									 IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) 
{
	printf("\n-+--- OptionsTemplate (id=%u)\n", optionsTemplateInfo->templateId);
	printf(" `---\n\n");
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID, 
												IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) 
{
	printf("Destroyed an OptionsTemplate (id=%u)\n", optionsTemplateInfo->templateId);
}

/**
 * Prints an OptionsRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixPrinter::onOptionsRecord(IpfixRecord::SourceID* sourceID, 
										   IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, 
										   uint16_t length, IpfixRecord::Data* data) 
{
	printf("\n-+--- OptionsDataRecord (Template id=%u)\n", optionsTemplateInfo->templateId);
	printf(" `---\n\n");
}

/**
 * Prints a DataTemplate
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onDataTemplate(IpfixRecord::SourceID* sourceID, 
								  IpfixRecord::DataTemplateInfo* dataTemplateInfo) 
{
	int i;

	printf("\n-+--- DataTemplate (id=%u)\n", dataTemplateInfo->templateId);
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
void IpfixPrinter::onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, 
											 IpfixRecord::DataTemplateInfo* dataTemplateInfo) 
{
	printf("Destroyed a DataTemplate (id=%u)\n", dataTemplateInfo->templateId);
}

/**
 * Prints a DataDataRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
void IpfixPrinter::onDataDataRecord(IpfixRecord::SourceID* sourceID, 
									IpfixRecord::DataTemplateInfo* dataTemplateInfo, 
									uint16_t length, IpfixRecord::Data* data) 
{
	int i;

	printf("\n-+--- DataDataRecord (Template id=%u)\n", dataTemplateInfo->templateId);
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `- variable data\n");
	for (i = 0; i < dataTemplateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->fieldInfo[i].type, (data + dataTemplateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
}

