/*
 * IPFIX Carmentis Exporter Module
 * Copyright (C) 2010 Vermont Project
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

#include "IpfixCsExporter.hpp"
#include "core/Timer.h"
#include <sys/stat.h>

/**
 * Creates a new IPFIXCsExporter.
 */
IpfixCsExporter::IpfixCsExporter(std::string filenamePrefix,
		std::string destinationPath, uint32_t maximumFilesize, uint32_t maxChunkBufferTime,
		uint32_t maxChunkBufferRecords, uint32_t maxFileCreationInterval, uint8_t exportMode)
{
	//fill configuration variables
	this->filenamePrefix = filenamePrefix;
	this->destinationPath = destinationPath;
	this->maxFileSize = maximumFilesize;
	this->maxChunkBufferTime = maxChunkBufferTime;
	this->maxChunkBufferRecords = maxChunkBufferRecords;
	this->maxFileCreationInterval = maxFileCreationInterval;
	this->exportMode = exportMode;
	currentFile = NULL;
	currentFileSize = 0;

	//Register first timeouts
	nextChunkTimeout.tv_sec = 0;
	nextChunkTimeout.tv_nsec = 0;
	nextFileTimeout.tv_sec = 0;
	nextFileTimeout.tv_nsec = 0;
	timeoutRegistered=false;

	CS_IPFIX_MAGIC[0] = 0xCA;
	memcpy(&CS_IPFIX_MAGIC[1], "CSIPFIX", 7);

	msg(MSG_INFO, "IpfixCsExporter initialized with the following parameters");
	msg(MSG_INFO, "  - filenamePrefix = %s" , filenamePrefix.c_str());
	msg(MSG_INFO, "  - destinationPath = %s", destinationPath.c_str());
	msg(MSG_INFO, "  - maxFileSize = %d KiB" , maxFileSize);
	msg(MSG_INFO, "  - maxChunkBufferTime = %d seconds" , maxChunkBufferTime);
	msg(MSG_INFO, "  - maxChunkBufferRecords = %d seconds" , maxChunkBufferRecords);
	msg(MSG_INFO, "  - maxFileCreationInterval = %d seconds" , maxFileCreationInterval);
	msg(MSG_INFO, "  - exportMode = %d" , exportMode);
	msg(MSG_INFO, "IpfixCsExporter: running");
}

IpfixCsExporter::~IpfixCsExporter()
{
}

/**
 * adds information to cs-record structure and pushing into chained list
 */
void IpfixCsExporter::onDataRecord(IpfixDataRecord* record)
{
	if ((record->templateInfo->setId != TemplateInfo::NetflowTemplate)
			&& (record->templateInfo->setId != TemplateInfo::IpfixTemplate)
			&& (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
		record->removeReference();
		return;
	}

	//create a new Ipfix_basic_flow and fill it with data
	Ipfix_basic_flow* csRecord = new Ipfix_basic_flow();
	TemplateInfo::FieldInfo* fi;

	csRecord->record_length			= htons(sizeof(Ipfix_basic_flow));		/* total length of this record in bytes */
	csRecord->src_export_mode		= exportMode;				/* expected to match enum cs_export_mode */
	csRecord->dst_export_mode		= exportMode;				/* expected to match enum cs_export_mode */
	csRecord->ipversion				= 4;						/* expected 4 (for now) */

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4Address, 0);
	if (fi != 0) {
		csRecord->source_ipv4_address		= *(uint32_t*)(record->data + fi->offset);
	} else {
		msg(MSG_DEBUG, "failed to determine source ip for record, assuming 0.0.0.0");
		csRecord->source_ipv4_address		= 0;
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4Address, 0);
	if (fi != 0) {
		csRecord->destination_ipv4_address	= *(uint32_t*)(record->data + fi->offset);
	} else {
		msg(MSG_DEBUG, "failed to determine destination ip for record, assuming 0.0.0.0");
		csRecord->destination_ipv4_address	= 0;
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_protocolIdentifier, 0);
	if (fi != 0) {
		csRecord->protocol_identifier 		= *(uint8_t*)(record->data + fi->offset);
	} else {
		msg(MSG_DEBUG, "failed to determine protocol for record, using 0");
		csRecord->protocol_identifier		= 0;
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_sourceTransportPort, 0);
        if (fi != 0) {
		csRecord->source_transport_port		= *(uint16_t*)(record->data + fi->offset);/* encode udp/tcp ports here */
	} else {
		msg(MSG_DEBUG, "failed to determine source port for record, assuming 0");
		csRecord->source_transport_port		= 0;
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_destinationTransportPort, 0);
	if (fi != 0) {
		csRecord->destination_transport_port	= *(uint16_t*)(record->data + fi->offset);/* encode udp/tcp ports here */
	} else {
		msg(MSG_DEBUG, "failed to determine destination port for record, assuming 0");
		csRecord->destination_transport_port	= 0;
	}

	// IPFIX_TYPEID_icmpTypeCode   (ICMP type * 256) + ICMP code (network-byte order!)
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_icmpTypeCode, 0);
	if (fi != 0) {
		csRecord->icmp_type_ipv4 		= *(uint8_t*)(record->data + fi->offset);
		csRecord->icmp_code_ipv4		= *(uint8_t*)(record->data + fi->offset + 8);
	} else {
		msg(MSG_DEBUG, "failed to determine icmp type and code for record, assuming 0");
		csRecord->icmp_type_ipv4                = 0;
		csRecord->icmp_code_ipv4                = 0;
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_tcpControlBits, 0);
        if (fi != 0) {
		csRecord->tcp_control_bits = *(uint8_t*)(record->data + fi->offset);
	}

	uint64_t srcTimeStart;
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartNanoSeconds, 0);
	if (fi != 0) {
		convertNtp64(*(uint64_t*)(record->data + fi->offset), srcTimeStart);
	} else {
		fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartMilliSeconds, 0);
		if (fi != 0) {
			srcTimeStart = ntohll(*(uint64_t*)(record->data + fi->offset));
		} else {
			fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartSeconds, 0);
			if (fi != 0) {
				srcTimeStart = ntohl(*(uint32_t*)(record->data + fi->offset));
				srcTimeStart *= 1000;
			} else {
				srcTimeStart = 0;
			}
		}
	}
	csRecord->flow_start_milliseconds = htonll(srcTimeStart);

	uint64_t srcTimeEnd;
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revFlowStartNanoSeconds, 0);
	if (fi != 0) {
		convertNtp64(*(uint64_t*)(record->data + fi->offset), srcTimeEnd);
	} else {
		fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndMilliSeconds, 0);
		if (fi != 0) {
			srcTimeEnd = ntohll(*(uint64_t*)(record->data + fi->offset));
		} else {
			fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowEndSeconds, 0);
			if (fi != 0) {
				srcTimeEnd = ntohl(*(uint32_t*)(record->data + fi->offset));
				srcTimeEnd *= 1000;
			} else {
				srcTimeEnd = 0;
			}
		}
	}
	csRecord->flow_end_milliseconds = htonll(srcTimeEnd);

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_octetDeltaCount, 0);
        if (fi != 0) {
		csRecord->octet_total_count = *(uint64_t*)(record->data + fi->offset);
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_packetDeltaCount, 0);
	if (fi != 0) {
		csRecord->packet_total_count = *(uint64_t*)(record->data + fi->offset);
	}

	csRecord->biflow_direction = 0;

	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revOctetDeltaCount, 0);
	if (fi != 0) {
		csRecord->rev_octet_total_count = *(uint64_t*)(record->data + fi->offset);
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revPacketDeltaCount, 0);
	if (fi != 0) {
		csRecord->rev_packet_total_count = *(uint64_t*)(record->data + fi->offset);
	}

	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revTcpControlBits, 0);
	if (fi != 0) {
		csRecord->rev_tcp_control_bits = *(uint8_t*)(record->data + fi->offset);
	}

	//add data to linked list
	chunkList.push_back(csRecord);

	//check if maxChunkBufferRecords is reached
	if(chunkList.size() == maxChunkBufferRecords)
		writeChunkList();

	//check if maxFileSize is reached
	currentFileSize += sizeof(Ipfix_basic_flow);
	if(currentFileSize > maxFileSize*1024){
		//close File, add new one
		writeChunkList();
		writeFileHeader();
	}
	record->removeReference();
}


void IpfixCsExporter::onTimeout(void* dataPtr)
{
	timeoutRegistered = false;
	struct timeval now;
	gettimeofday(&now, 0);
	//check if this is one of the desired timeouts
	if (nextFileTimeout.tv_sec <= now.tv_sec) {
		//close File, add new one
		writeChunkList();
		writeFileHeader();
		addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
		addToCurTime(&nextFileTimeout, maxFileCreationInterval*1000);
	} else if (nextChunkTimeout.tv_sec <= now.tv_sec) {
		writeChunkList();
		addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
	}

	registerTimeout();
}

/**
 * Creates an output file and writes the CS_Ipfix_file_header
 */
void IpfixCsExporter::writeFileHeader()
{
	if (currentFile != NULL) {
		fclose(currentFile);
	}
	currentFileSize = sizeof(CS_IPFIX_MAGIC)+sizeof(Ipfix_basic_flow_sequence_chunk_header);

	time_t timestamp = time(0);
	tm *st = localtime(&timestamp);

	struct stat sta;

	char time[512];
	sprintf(time, "%s%s%02d%02d%02d-%02d%02d",destinationPath.c_str(), filenamePrefix.c_str(), st->tm_year+1900,st->tm_mon+1,st->tm_mday,st->tm_hour,st->tm_min);
	uint32_t i = 1;
	while (i<0xFFFFFFFE) {
		sprintf(currentFilename, "%s_%03d",time,i);
		errno = 0;
		if (stat(currentFilename,&sta) != 0) {
			if (errno != 2) {
				//check error code
				msg(MSG_ERROR, "IpfixCsExporter: stat() on filename %s returned with error %i (%s)", currentFilename, errno, strerror(errno));
			} else {
				// errno==2 means there is no file present
				break;
			}
		}
		i++;
	}

	if (i==0xFFFFFFFF) {
		THROWEXCEPTION("failed to determine index for filename postfix (i==0xFFFFFFFF). Something went terribly wrong ....");
	}

	currentFile = fopen(currentFilename, "wb");
	if (currentFile == NULL) {
		THROWEXCEPTION("Could not open file for writing. Check permissions.");
	}

	if (fwrite(CS_IPFIX_MAGIC, sizeof(CS_IPFIX_MAGIC), 1, currentFile)==0){
		THROWEXCEPTION("Could not write file header. Check disk space.");
	}

	//new Timeouts:
	addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
	addToCurTime(&nextFileTimeout, maxFileCreationInterval*1000);
}

/**
 * Writes the content of chunkList to the output file
 */
void IpfixCsExporter::writeChunkList()
{
	Ipfix_basic_flow_sequence_chunk_header csChunkHeader;

	csChunkHeader.ipfix_type = htons(0x0008);
	csChunkHeader.chunk_length = htonl(chunkList.size()*sizeof(Ipfix_basic_flow));
	msg(MSG_INFO, "chunk_length: %u, %X", chunkList.size()*sizeof(Ipfix_basic_flow), csChunkHeader.chunk_length);
	csChunkHeader.flow_count = htonl(chunkList.size());

	if (fwrite(&csChunkHeader, sizeof(csChunkHeader), 1, currentFile)==0){
		THROWEXCEPTION("Could not chunk header. Check disk space.");
	}

	msg(MSG_DEBUG, "IpfixCsExporter: writing %u records to disk", chunkList.size());

	while (chunkList.size() > 0){
		if (fwrite(chunkList.front(), sizeof(Ipfix_basic_flow), 1, currentFile)==0){
			THROWEXCEPTION("Could not write basic flow data. Check disk space.");
		}

		chunkList.pop_front();
	}
	addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
}


/**
 * Registers timeout for function onTimeout in Timer
 * to compute the criterias for every interval
 */
void IpfixCsExporter::registerTimeout()
{
	if (timeoutRegistered) return;
	if (nextChunkTimeout.tv_sec <= nextFileTimeout.tv_sec){
		// Register a chunk timeout
		timer->addTimeout(this, nextChunkTimeout, NULL);
		msg(MSG_DEBUG, "next timeout: %u", nextChunkTimeout.tv_sec);
	} else {
		// register a file timeout
		timer->addTimeout(this, nextFileTimeout, NULL);
		msg(MSG_DEBUG, "next timeout: %u", nextFileTimeout.tv_sec);
	}

	timeoutRegistered = true;
}

/**
 * Start function
 */
void IpfixCsExporter::performStart()
{
	writeFileHeader();
	addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
	addToCurTime(&nextFileTimeout, maxFileCreationInterval*1000);
	registerTimeout();
}


void IpfixCsExporter::performShutdown()
{
	if (currentFile != NULL) {
		writeChunkList();
		fclose(currentFile);
	}
}
