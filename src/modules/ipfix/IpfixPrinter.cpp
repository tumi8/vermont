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

#include "IpfixPrinter.hpp"
#include "common/Time.h"
#include "common/Misc.h"
#include "Connection.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>

/**
 * print functions which have formerly been in IpfixParser.cpp
 */

static void printIPv4(InformationElement::IeInfo type, IpfixRecord::Data* data) {
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

static void printPort(InformationElement::IeInfo type, IpfixRecord::Data* data) {
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

void printProtocol(InformationElement::IeInfo type, IpfixRecord::Data* data) {
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

static void printUint(InformationElement::IeInfo type, IpfixRecord::Data* data) {
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
void printFieldData(InformationElement::IeInfo type, IpfixRecord::Data* pattern) {
	char* s;
	timeval t;
	uint64_t hbnum;

	if(type.enterprise == 0) {
		switch (type.id) {
		case IPFIX_TYPEID_protocolIdentifier:
			printf("protocolIdentifier: ");
			printProtocol(type, pattern);
			return;
		case IPFIX_TYPEID_sourceIPv4Address:
			printf("sourceIPv4Address: ");
			printIPv4(type, pattern);
			return;
		case IPFIX_TYPEID_destinationIPv4Address:
			printf("destinationIPv4Address: ");
			printIPv4(type, pattern);
			return;
		case IPFIX_TYPEID_sourceTransportPort:
			printf("sourceTransportPort: ");
			printPort(type, pattern);
			return;
		case IPFIX_TYPEID_destinationTransportPort:
			printf("destinationTransportPort: ");
			printPort(type, pattern);
			return;
		case IPFIX_TYPEID_flowStartNanoSeconds:
		case IPFIX_TYPEID_flowEndNanoSeconds:
		// TODO: replace by enterprise number (Gerhard, 12/2009)
		case IPFIX_ETYPEID_revFlowStartNanoSeconds:
		case IPFIX_ETYPEID_revFlowEndNanoSeconds:
			printf("%s: ", typeid2string(type.id));
			hbnum = ntohll(*(uint64_t*)pattern);
			if (hbnum>0) {
				t = timentp64(*((ntp64*)(&hbnum)));
				printf("%d.%06d seconds", (int32_t)t.tv_sec, (int32_t)t.tv_usec);
			} else {
				printf("no value (only zeroes in field)");
			}
			return;
		case IPFIX_ETYPEID_frontPayload:
		case IPFIX_ETYPEID_revFrontPayload:
			printf("%s: ", typeid2string(type.id));
			printFrontPayload(type, pattern);
			return;
		}
	}
	// default treatment
	s = typeid2string(type.id);
	if (s != NULL) {
		printf("%s: ", s);
		printUint(type, pattern);
	} else {
		DPRINTF("Field with ID %d unparseable\n", type.id);
	}
}


void printFrontPayload(InformationElement::IeInfo type, IpfixRecord::Data* data)
{
	for (uint32_t i=0; i<type.length; i++) {
		char c = data[i];
		if (isprint(c)) printf("%c", c);
		else printf(".");
	}
	printf("'");
}


/**
 * Creates a new IpfixPrinter. Do not forget to call @c startIpfixPrinter() to begin printing
 * @return handle to use when calling @c destroyIpfixPrinter()
 */
IpfixPrinter::IpfixPrinter(OutputType outputtype, string filename)
	: linesPrinted(0), outputType(outputtype), filename(filename)
{
	lastTemplate = 0;

	msg(MSG_INFO, "IpfixPrinter started with following parameters:");
	string type;
	switch (outputtype) {
		case TREE: type = "tree"; break;
		case LINE: type = "line"; break;
		case TABLE: type = "table"; break;
		case NONE: type = "no output"; break;
	}
	msg(MSG_INFO, "  - outputType=%s", type.c_str());
	string file = "standard output";
	if (filename!="") file = "in file '" + filename + "'";
	msg(MSG_INFO, "  - output=%s", file.c_str());

	fh = stdout;
	if (filename != "") {
		fh = fopen(filename.c_str(), "w");
		if (!fh)
			THROWEXCEPTION("IpfixPrinter: error opening file '%s': %s (%d)", filename.c_str(), strerror(errno), errno);
	}

	if (outputtype==TABLE)
		fprintf(fh, "c.srcOctets\tc.srcPackets\tc.srcPayloadLen\tc.srcPayloadPktCount\tc.srcTimeEnd-c.srcTimeStart\n");
}

/**
 * Frees memory used by an IpfixPrinter
 */
IpfixPrinter::~IpfixPrinter()
{
	if (filename != "") {
		int ret = fclose(fh);
		if (ret)
			THROWEXCEPTION("IpfixPrinter: error closing file '%s': %s (%d)", filename.c_str(), strerror(errno), errno);
	}
}


/**
 * Prints a Template
 * @param sourceID SourceID of the exporting process
 * @param templateInfo Pointer to a structure defining the Template used
 */
void IpfixPrinter::onTemplate(IpfixTemplateRecord* record)
{
	/* we need a FieldInfo for printIPv4 */
	InformationElement::IeInfo tmpInfo = {0, 4, 0}; // length=4 for IPv4 address
	switch(record->templateInfo->setId) {
		case TemplateInfo::NetflowTemplate:
			printf("\n-+--- Netflow Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::NetflowOptionsTemplate:
			printf("\n-+--- Netflow Options Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixTemplate:
			printf("\n-+--- Ipfix Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixOptionsTemplate:
			printf("\n-+--- Ipfix Options Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixDataTemplate:
			printf("\n-+--- Ipfix Data Template (id=%u) from ", record->templateInfo->templateId);
			break;
		default:
			msg(MSG_ERROR, "IpfixPrinter: Template with unknown setid=%d", record->templateInfo->setId);

	}
	if (record->sourceID) {
		if (record->sourceID->exporterAddress.len == 4)
			printIPv4(tmpInfo, &record->sourceID->exporterAddress.ip[0]);
		else
			printf("non-IPv4 address");
		printf(":%d (", record->sourceID->exporterPort);
		tmpInfo.length = 1; // length=1 for protocol identifier
		printProtocol(tmpInfo, &record->sourceID->protocol);
		printf(")\n");
	} else {
		printf("no sourceID given in template");
	}

	// print fixed data in the case of a data template
	if(record->templateInfo->setId == TemplateInfo::IpfixDataTemplate) {
		printf(" `- fixed data\n");
		for (int i = 0; i < record->templateInfo->dataCount; i++) {
			printf(" '   `- ");
			printFieldData(record->templateInfo->dataInfo[i].type,
					(record->templateInfo->data + record->templateInfo->dataInfo[i].offset));
			printf("\n");
		}
	}
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
	/* we need a FieldInfo for printIPv4 */
	InformationElement::IeInfo tmpInfo = {0, 4, 0}; // length=4 for IPv4 address
	switch(record->templateInfo->setId) {
		case TemplateInfo::NetflowTemplate:
			printf("\n-+--- Destroyed Netflow Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::NetflowOptionsTemplate:
			printf("\n-+--- Destroyed Netflow Options Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixTemplate:
			printf("\n-+--- Destroyed Ipfix Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixOptionsTemplate:
			printf("\n-+--- Destroyed Ipfix Options Template (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixDataTemplate:
			printf("\n-+--- Destroyed Ipfix Data Template (id=%u) from ", record->templateInfo->templateId);
			break;

	}
	if (record->sourceID) {
		if (record->sourceID->exporterAddress.len == 4)
			printIPv4(tmpInfo, &record->sourceID->exporterAddress.ip[0]);
		else
			printf("non-IPv4 address");
		printf(":%d (", record->sourceID->exporterPort);
		tmpInfo.length = 1; // length=1 for protocol identifier
		printProtocol(tmpInfo, &record->sourceID->protocol);
		printf(")\n");
	} else {
		printf("no sourceID given in template");
	}
	record->removeReference();
}


void IpfixPrinter::printUint(char* buf, InformationElement::IeInfo type, IpfixRecord::Data* data) {
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
 * prints a datarecord in a special, easy-to-read data format in one line
 */
void IpfixPrinter::printOneLineRecord(IpfixDataRecord* record)
{
	boost::shared_ptr<TemplateInfo> dataTemplateInfo = record->templateInfo;
		char buf[100], buf2[100];

		if (linesPrinted==0 || linesPrinted>50) {
			printf("%22s %20s %8s %5s %21s %21s %5s %5s\n", "Flow recvd.", "Flow start", "Duratn", "Prot", "Src IP:Port", "Dst IP:Port", "Pckts", "Bytes");
			printf("-----------------------------------------------------------------------------------------------------------------\n");
			linesPrinted = 0;
		}
		struct tm* tm;
		struct timeval tv;
		gettimeofday(&tv, 0);
		tm = localtime(reinterpret_cast<time_t*>(&tv.tv_sec));
		strftime(buf, ARRAY_SIZE(buf), "%F %T", tm);
		snprintf(buf2, ARRAY_SIZE(buf2), "%s.%03ld", buf, tv.tv_usec/1000);
		printf("%22s ", buf2);

		uint32_t timetype = 0;
		uint32_t starttime = 0;
		TemplateInfo::FieldInfo* fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSeconds, 0);
		if (fi != NULL) {
			timetype = IPFIX_TYPEID_flowStartSeconds;
			time_t t = ntohl(*reinterpret_cast<time_t*>(record->data+fi->offset));
			starttime = t;
			tm = localtime(&t);
			strftime(buf, 50, "%F %T", tm);
		} else {
			fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowStartMilliSeconds, 0);
			if (fi != NULL) {
				timetype = IPFIX_TYPEID_flowStartMilliSeconds;
				uint64_t t2 = ntohll(*reinterpret_cast<uint64_t*>(record->data+fi->offset));
				time_t t = t2/1000;
				starttime = t;
				tm = localtime(&t);
				strftime(buf, 50, "%F %T", tm);
			} else {
				fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSysUpTime, 0);
				if (fi != NULL) {
					timetype = IPFIX_TYPEID_flowStartSysUpTime;
					starttime = ntohl(*reinterpret_cast<uint32_t*>(record->data+fi->offset));
					snprintf(buf, 50, "%u:%02u.%04u", starttime/60000, (starttime%60000)/1000, starttime%1000);
				} else {
					fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSeconds, 0);
					if (fi != NULL) {
						timetype = IPFIX_TYPEID_flowStartNanoSeconds;
						uint64_t t2 = ntohll(*reinterpret_cast<uint64_t*>(record->data+fi->offset));
						timeval t = timentp64(*((ntp64*)(&t2)));
						tm = localtime(&t.tv_sec);
						strftime(buf, 50, "%F %T", tm);
						starttime = t.tv_sec;
					}
				}
			}
		}
		if (timetype != 0) {
			printf("%20s ", buf);

			uint32_t dur = 0;
			switch (timetype) {
				case IPFIX_TYPEID_flowStartSeconds:
					fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowEndSeconds, 0);
					if (fi != NULL) {
						dur = ntohl(*reinterpret_cast<uint32_t*>(record->data+fi->offset)) - starttime;
						dur *= 1000;
					}
					break;
				case IPFIX_TYPEID_flowStartMilliSeconds:
					fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowEndMilliSeconds, 0);
					if (fi != NULL) {
						dur = ntohll(*reinterpret_cast<uint64_t*>(record->data+fi->offset)) - starttime;
						dur *= 1000;
					}
					break;
				case IPFIX_TYPEID_flowStartSysUpTime:
					fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowEndSysUpTime, 0);
					if (fi != NULL) {
						dur = ntohl(*reinterpret_cast<uint32_t*>(record->data+fi->offset)) - starttime;
						dur *= 1000;
					}
					break;
				case IPFIX_TYPEID_flowStartNanoSeconds:
					fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_flowEndNanoSeconds, 0);
					if (fi != NULL) {
						uint64_t t2 = ntohll(*reinterpret_cast<uint64_t*>(record->data+fi->offset));
						timeval t = timentp64(*((ntp64*)(&t2)));
						dur = t.tv_sec*1000+t.tv_usec/1000 - starttime;
					}
			}
			snprintf(buf, 50, "%u.%04u", (dur)/1000, dur%1000);
			printf("%8s ", buf);
		}
		else {
			printf("%20s %8s ", "---", "---");
		}

		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_protocolIdentifier, 0);
		if (fi != NULL && fi->type.length==1) {
			snprintf(buf, ARRAY_SIZE(buf), "%hhu", *reinterpret_cast<uint8_t*>(record->data+fi->offset));
		} else {
			snprintf(buf, ARRAY_SIZE(buf), "---");
		}
		printf("%5s ", buf);

		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4Address, 0);
		uint32_t srcip = 0;
		if (fi != NULL && fi->type.length>=4) {
			srcip = *reinterpret_cast<uint32_t*>(record->data+fi->offset);
		}
		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_sourceTransportPort, 0);
		uint32_t srcport = 0;
		if (fi != NULL && fi->type.length==2) {
			srcport = *reinterpret_cast<uint16_t*>(record->data+fi->offset);
		}
		snprintf(buf, ARRAY_SIZE(buf), "%hhu.%hhu.%hhu.%hhu:%hu", (srcip>>0)&0xFF, (srcip>>8)&0xFF, (srcip>>16)&0xFF, (srcip>>24)&0xFF, srcport);
		printf("%21s ", buf);

		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4Address, 0);
		uint32_t dstip = 0;
		if (fi != NULL && fi->type.length>=4) {
			dstip = *reinterpret_cast<uint32_t*>(record->data+fi->offset);
		}
		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_destinationTransportPort, 0);
		uint32_t dstport = 0;
		if (fi != NULL && fi->type.length==2) {
			dstport = *reinterpret_cast<uint16_t*>(record->data+fi->offset);
		}
		snprintf(buf, ARRAY_SIZE(buf), "%hhu.%hhu.%hhu.%hhu:%hu", (dstip>>0)&0xFF, (dstip>>8)&0xFF, (dstip>>16)&0xFF, (dstip>>24)&0xFF, dstport);
		printf("%21s ", buf);

		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_packetDeltaCount, 0);
		if (fi != NULL) {
			printUint(buf, fi->type, record->data+fi->offset);
		} else {
			snprintf(buf, ARRAY_SIZE(buf), "---");
		}
		printf("%5s ", buf);

		fi = dataTemplateInfo->getFieldInfo(IPFIX_TYPEID_octetDeltaCount, 0);
		if (fi != NULL) {
			printUint(buf, fi->type, record->data+fi->offset);
		} else {
			snprintf(buf, ARRAY_SIZE(buf), "---");
		}
		printf("%5s \n", buf);
		linesPrinted++;
}

/**
 * prints record as a tree
 */
void IpfixPrinter::printTreeRecord(IpfixDataRecord* record)
{
	/* we need a FieldInfo for printIPv4 */
	InformationElement::IeInfo tmpInfo = {0, 4, 0}; // length=4 for IPv4 address
	switch(record->templateInfo->setId) {
		case TemplateInfo::NetflowTemplate:
			printf("\n-+--- Netflow Data Record (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::NetflowOptionsTemplate:
			printf("\n-+--- Netflow Options Data Record (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixTemplate:
			printf("\n-+--- Ipfix Data Record (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixOptionsTemplate:
			printf("\n-+--- Ipfix Options Data Record (id=%u) from ", record->templateInfo->templateId);
			break;
		case TemplateInfo::IpfixDataTemplate:
			printf("\n-+--- Ipfix Data Data Record (id=%u) from ", record->templateInfo->templateId);
			break;
		default:
			msg(MSG_ERROR, "IpfixPrinter: Template with unknown setid=%d", record->templateInfo->setId);

	}
	if (record->sourceID) {
		if (record->sourceID->exporterAddress.len == 4)
			printIPv4(tmpInfo, &record->sourceID->exporterAddress.ip[0]);
		else
			printf("non-IPv4 address");
		printf(":%d (", record->sourceID->exporterPort);
		tmpInfo.length = 1; // length=1 for protocol identifier
		printProtocol(tmpInfo, &record->sourceID->protocol);
		printf(")\n");
	} else {
		printf("no sourceID given");
	}

	if(record->templateInfo->setId == TemplateInfo::IpfixDataTemplate) {
		printf(" `- fixed data\n");
		for (i = 0; i < record->templateInfo->dataCount; i++) {
			printf(" '   `- ");
			printFieldData(record->templateInfo->dataInfo[i].type,
					(record->templateInfo->data + record->templateInfo->dataInfo[i].offset));
			printf("\n");
		}
	}

	if(record->templateInfo->setId == TemplateInfo::IpfixOptionsTemplate) {
		printf(" `- variable scope data\n");
		for(i = 0; i < record->templateInfo->scopeCount; i++) {
			printf(" '   `- ");
			printFieldData(record->templateInfo->scopeInfo[i].type, (record->data + record->templateInfo->scopeInfo[i].offset));
			printf("\n");
		}
	}
	printf(" `- variable data\n");
	for (i = 0; i < record->templateInfo->fieldCount; i++) {
		printf(" '   `- ");
		printFieldData(record->templateInfo->fieldInfo[i].type, (record->data + record->templateInfo->fieldInfo[i].offset));
		printf("\n");
	}
	printf(" `---\n\n");
}

/**
 * prints tab-seperated data from flows, these may be specified in configuration (TODO!)
 */
void IpfixPrinter::printTableRecord(IpfixDataRecord* record)
{
	Connection c(record);

	//fprintf(fh, "%llu\t%llu\t%u\t%u\t%llu\n", ntohll(c.srcOctets), ntohll(c.srcPackets), c.srcPayloadLen, c.srcPayloadPktCount, c.srcTimeEnd-c.srcTimeStart);
	fprintf(fh, "%s\t%s\t%hu\t%hu\t%hhu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%llu\t%hhu\t%hhu\n",
			IPToString(c.srcIP).c_str(), IPToString(c.dstIP).c_str(), ntohs(c.srcPort), ntohs(c.dstPort), c.protocol,
			ntohll(c.srcPackets), ntohll(c.dstPackets), ntohll(c.srcOctets), ntohll(c.dstOctets),
			c.srcTimeStart, c.srcTimeEnd, c.dstTimeStart, c.dstTimeEnd, c.srcTcpControlBits, c.dstTcpControlBits);

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
	/* we need a FieldInfo for printIPv4 */
	InformationElement::IeInfo tmpInfo = {0, 4, 0}; // length=4 for IPv4 address

	switch (outputType) {
		case LINE:
			printOneLineRecord(record);
			break;
		case TREE:
			printTreeRecord(record);
			break;

		case TABLE:
			printTableRecord(record);
			break;
		case NONE:
			break;
	}

	record->removeReference();
}


