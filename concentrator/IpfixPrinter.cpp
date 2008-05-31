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
 * print functions which have formerly been in IpfixParser.cpp
 */
	
static void printIPv4(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data) {
	int octet1 = 0;
	int octet2 = 0;
	int octet3 = 0;
	int octet4 = 0;
	int imask = 0;
	if (type.length >= 1) octet1 = data[0];
	if (type.length >= 2) octet2 = data[1];
	if (type.length >= 3) octet3 = data[2];
	if (type.length >= 4) octet4 = data[3];
	if (type.length >= 5) imask = data[4];
	if (type.length > 5) {
		DPRINTF("IPv4 Address with length %d unparseable\n", type.length);
		return;
	}

	if ((type.length == 5) /*&& (imask != 0)*/) {
		printf("%d.%d.%d.%d/%d", octet1, octet2, octet3, octet4, 32-imask);
	} else {
		printf("%d.%d.%d.%d", octet1, octet2, octet3, octet4);
	}
}

static void printPort(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data) {
	if (type.length == 0) {
		printf("zero-length Port");
		return;
	}
	if (type.length == 2) {
		int port = ((uint16_t)data[0] << 8)+data[1];
		printf("%d", port);
		return;
	}
	if ((type.length >= 4) && ((type.length % 4) == 0)) {
		int i;
		for (i = 0; i < type.length; i+=4) {
			int starti = ((uint16_t)data[i+0] << 8)+data[i+1];
			int endi = ((uint16_t)data[i+2] << 8)+data[i+3];
			if (i > 0) printf(",");
			if (starti != endi) {
				printf("%d:%d", starti, endi);
			} else {
				printf("%d", starti);
			}
		}
		return;
	}

	printf("Port with length %d unparseable", type.length);
}

void printProtocol(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data) {
	if (type.length != 1) {
		printf("Protocol with length %d unparseable", type.length);
		return;
	}
	switch (data[0]) {
	case IPFIX_protocolIdentifier_ICMP:
		printf("ICMP");
		return;
	case IPFIX_protocolIdentifier_TCP:
		printf("TCP");
		return;
	case IPFIX_protocolIdentifier_UDP: 
		printf("UDP");
		return;
	case IPFIX_protocolIdentifier_SCTP: 
		printf("SCTP");
		return;
	case IPFIX_protocolIdentifier_RAW: 
		printf("RAW");
		return;
	default:
		printf("unknownProtocol");
		return;
	}
}

static void printUint(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* data) {
	switch (type.length) {
	case 1:
		printf("%hhu",*(uint8_t*)data);
		return;
	case 2:
		printf("%hu",ntohs(*(uint16_t*)data));
		return;
	case 4:
		printf("%u",ntohl(*(uint32_t*)data));
		return;
	case 8:
		printf("%Lu",ntohll(*(uint64_t*)data));
		return;
	default:
		for(uint16_t i = 0; i < type.length; i++) {
		    printf("%02hhX",*(uint8_t*)(data+i));
		}
		printf(" (%u bytes)", type.length);
		//msg(MSG_ERROR, "Uint with length %d unparseable", type.length);
		return;
	}
}


/**
 * Prints a string representation of IpfixRecord::Data to stdout.
 */
void printFieldData(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* pattern) {
	char* s;

	switch (type.id) {
	case IPFIX_TYPEID_protocolIdentifier:
		printf("protocolIdentifier: ");
		printProtocol(type, pattern);
		break;
	case IPFIX_TYPEID_sourceIPv4Address:
		printf("sourceIPv4Address: ");
		printIPv4(type, pattern);
		break;
	case IPFIX_TYPEID_destinationIPv4Address:
		printf("destinationIPv4Address: ");
		printIPv4(type, pattern);
		break;
	case IPFIX_TYPEID_sourceTransportPort:
		printf("sourceTransportPort: ");
		printPort(type, pattern);
		break;
	case IPFIX_TYPEID_destinationTransportPort:
		printf("destinationTransportPort: ");
		printPort(type, pattern);
		break;
	default:
		s = typeid2string(type.id);
		if (s != NULL) {
			printf("%s: ", s);
			printUint(type, pattern);
		} else {
			DPRINTF("Field with ID %d unparseable\n", type.id);
		}
		break;
	}
}


/**
 * Creates a new IpfixPrinter. Do not forget to call @c startIpfixPrinter() to begin printing
 * @return handle to use when calling @c destroyIpfixPrinter()
 */
IpfixPrinter::IpfixPrinter() {
	lastTemplate = 0;	
	setSinkOwner("IpfixPrinter");
}

/**
 * Frees memory used by an IpfixPrinter
 */
IpfixPrinter::~IpfixPrinter() {
}

/**
 * Starts or resumes printing messages
 */
void IpfixPrinter::start() {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Temporarily pauses printing messages
 */
void IpfixPrinter::stop() {
	/* unimplemented, we can't be paused - TODO: or should we? */
}

/**
 * Prints a Template
 * @param sourceID SourceID of the exporting process
 * @param templateInfo Pointer to a structure defining the Template used
 */
int IpfixPrinter::onTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) {
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("\n-+--- Template (id=%u) from ", templateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(")\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a Template that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixPrinter::onTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo) {
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("Destroyed a Template (id=%u) from ", templateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(")\n");

	return 0;
}

/**
 * Prints a DataRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int IpfixPrinter::onDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::TemplateInfo* templateInfo, uint16_t length, IpfixRecord::Data* data) {
	int i;
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("\n-+--- DataRecord (Template id=%u from ", templateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(") )\n");
	
	printf(" `- variable data\n");
	for (i = 0; i < templateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(templateInfo->fieldInfo[i].type, (data + templateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a OptionsTemplate
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixPrinter::onOptionsTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) {
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("\n-+--- OptionsTemplate (id=%u) from ", optionsTemplateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(")\n");
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixPrinter::onOptionsTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo) {
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("Destroyed an OptionsTemplate (id=%u) from ", optionsTemplateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(")\n");

	return 0;
}

/**
 * Prints an OptionsRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int IpfixPrinter::onOptionsRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::OptionsTemplateInfo* optionsTemplateInfo, uint16_t length, IpfixRecord::Data* data) {
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("\n-+--- OptionsDataRecord (Template id=%u from ", optionsTemplateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(") )\n");

	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixPrinter::onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo) {
	int i;
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("\n-+--- DataTemplate (id=%u) from ", dataTemplateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(")\n");
	
	printf(" `- fixed data\n");
	for (i = 0; i < dataTemplateInfo->dataCount; i++) {
		printf(" '   `- ");
		printFieldData(dataTemplateInfo->dataInfo[i].type, (dataTemplateInfo->data + dataTemplateInfo->dataInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");

	return 0;
}

/**
 * Prints a DataTemplate that was announced to be destroyed
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 */
int IpfixPrinter::onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo) {
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("Destroyed a DataTemplate (id=%u) from ", dataTemplateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(")\n");
	
	return 0;
}

/**
 * Prints a DataDataRecord
 * @param sourceID SourceID of the exporting process
 * @param dataTemplateInfo Pointer to a structure defining the DataTemplate used
 * @param length Length of the data block supplied
 * @param data Pointer to a data block containing all variable fields
 */
int IpfixPrinter::onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data) {
	int i;
	/* we need a FieldInfo for printIPv4 */
	IpfixRecord::FieldInfo::Type tmpInfo = {0, 4, false, 0}; // length=4 for IPv4 address
	printf("\n-+--- DataDataRecord (Template id=%u from ", dataTemplateInfo->templateId);
	if(sourceID->exporterAddress.len == 4)
		printIPv4(tmpInfo, &sourceID->exporterAddress.ip[0]);
	else
		printf("non-IPv4 address");
	printf(":%d (", sourceID->exporterPort);
	tmpInfo.length = 1; // length=1 for protocol identifier
	printProtocol(tmpInfo, &sourceID->protocol);
	printf(") )\n");
	
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

	return 0;
}

