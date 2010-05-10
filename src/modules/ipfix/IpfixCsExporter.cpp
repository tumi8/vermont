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
#include "common/ipfixlolib/ipfixlolib.h"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"
#include "core/Timer.h"
#include <stdexcept>
#include <string.h>


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

	//Register first timeouts
	addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
	addToCurTime(&nextFileTimeout, maxFileCreationInterval*1000);
	registerTimeout();
	//TODO: change currentFile
        currentFile=destinationPath+filenamePrefix+"filename";
	writeFileHeader();
	CS_IPFIX_MAGIC[0] = 0xCA;
	memcpy(&CS_IPFIX_MAGIC[1], "CSIPFIX", 7);

        msg(MSG_INFO, "IpfixCsExporter initialized with the following parameters");
        //msg(MSG_INFO, "  - Basename = %s", currentFile);
        msg(MSG_INFO, "  - maximumFilesize = %d KiB" , maxFileSize);
        msg(MSG_INFO, "  - maxChunkBufferTime = %d seconds" , maxChunkBufferTime);
        msg(MSG_INFO, "  - maxChunkBufferRecords = %d seconds" , maxChunkBufferRecords);
        msg(MSG_INFO, "  - maxFileCreationInterval = %d seconds" , maxFileCreationInterval);
        msg(MSG_INFO, "  - exportMode = %d" , exportMode);
	msg(MSG_DEBUG, "IpfixCsExporter: running");
}

IpfixCsExporter::~IpfixCsExporter() {
}

void IpfixCsExporter::onDataRecord(IpfixDataRecord* record){
	//Adding information to cs-record structure and pushing into chained list
        if((record->templateInfo->setId != TemplateInfo::NetflowTemplate)
                && (record->templateInfo->setId != TemplateInfo::IpfixTemplate)
                && (record->templateInfo->setId != TemplateInfo::IpfixDataTemplate)) {
                record->removeReference();
                return;
        }

	//create a new Ipfix_basic_flow and fill it with data
	Ipfix_basic_flow* csRecord = new Ipfix_basic_flow();
	TemplateInfo::FieldInfo* fi;

	csRecord->record_length			= sizeof(Ipfix_basic_flow);		/* total length of this record in bytes */
        csRecord->src_export_mode		= exportMode;				/* expected to match enum cs_export_mode */
        csRecord->dst_export_mode		= exportMode;				/* expected to match enum cs_export_mode */
        csRecord->ipversion			= 4;					/* expected 4 (for now) */

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_sourceIPv4Address, 0);
        csRecord->source_ipv4_address		= *(uint32_t*)(record->data + fi->offset);

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_destinationIPv4Address, 0);
        csRecord->destination_ipv4_address	= *(uint32_t*)(record->data + fi->offset);

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_protocolIdentifier, 0);
        csRecord->protocol_identifier 		= *(uint8_t*)(record->data + fi->offset);

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_sourceTransportPort, 0);
        csRecord->source_transport_port		= *(uint16_t*)(record->data + fi->offset);/* encode udp/tcp ports here */

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_destinationTransportPort, 0);
        csRecord->destination_transport_port	= *(uint16_t*)(record->data + fi->offset);/* encode udp/tcp ports here */

	// IPFIX_TYPEID_icmpTypeCode   (ICMP type * 256) + ICMP code (network-byte order!)
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_icmpTypeCode, 0);
        csRecord->icmp_type_ipv4 		= *(uint8_t*)(record->data + fi->offset);
	//TODO
        csRecord->icmp_code_ipv4		= 0;
	
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_tcpControlBits, 0);
        csRecord->tcp_control_bits = *(uint8_t*)(record->data + fi->offset);

	uint64_t srcTimeStart;
	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_flowStartNanoSeconds, 0);
        csRecord->flow_start_milliseconds = convertNtp64(*(uint64_t*)(record->data + fi->offset));

	uint64_t dstTimeStart;
	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revFlowStartNanoSeconds, 0);
        csRecord->flow_end_milliseconds = convertNtp64(*(uint64_t*)(record->data + fi->offset));;

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_octetDeltaCount, 0);
        csRecord->octet_total_count = *(uint64_t*)(record->data + fi->offset);

	fi = record->templateInfo->getFieldInfo(IPFIX_TYPEID_packetDeltaCount, 0);
        csRecord->packet_total_count = *(uint64_t*)(record->data + fi->offset);

        csRecord->biflow_direction = 0;

	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revOctetDeltaCount, 0);
        csRecord->rev_octet_total_count = *(uint64_t*)(record->data + fi->offset);

	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revPacketDeltaCount, 0);
        csRecord->rev_packet_total_count = *(uint64_t*)(record->data + fi->offset);

	fi = record->templateInfo->getFieldInfo(IPFIX_ETYPEID_revTcpControlBits, 0);
        csRecord->rev_tcp_control_bits = *(uint8_t*)(record->data + fi->offset);

	//add data to linked list
	chunkList.push_back(csRecord);
	
	//check if maxChunkBufferRecords is reached
	if(chunkList.size() == maxChunkBufferRecords)
		writeChunkList();

        //check if maxFileSize is reached
	FILE * stream = fopen("filename", "r");
	fseek(stream, 0L, SEEK_END);
	long fileSize = ftell(stream);
	fclose(stream);

	fileSize += sizeof(Ipfix_basic_flow_sequence_chunk_header)+(chunkList.size()+1)*sizeof(Ipfix_basic_flow);
	if(fileSize > maxFileSize*1024){
		//close File, add new one
		writeChunkList();
		//TODO: change currentFile
		currentFile=destinationPath+filenamePrefix+"filename";
		writeFileHeader();
	}
	record->removeReference();
}


void IpfixCsExporter::onTimeout(void* dataPtr)
{
	timeoutRegistered = false;
	//check if this is one of the desired timeouts
	if (nextFileTimeout.tv_sec <= 1) { // FIXME!
		//close File, add new one
                writeChunkList();
                //TODO: change currentFile
		// prefix_20100505-1515_001
                currentFile=destinationPath+filenamePrefix+"filename";
                writeFileHeader();
		addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
	        addToCurTime(&nextFileTimeout, maxFileCreationInterval*1000);
	} else if (nextChunkTimeout.tv_sec <= 1) { // FIXME!
                writeChunkList();
                addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
	}

	registerTimeout();
}


//FIXME: in Time.h verlagern, dann auch aus Connection.cpp entfernen!
uint64_t IpfixCsExporter::convertNtp64(uint64_t ntptime)
{
        uint64_t hbnum = ntohll(*(uint64_t*)&ntptime);
        if (hbnum>0) {
                timeval t = timentp64(*((ntp64*)(&hbnum)));
                return htonll((uint64_t)t.tv_sec*1000+(uint64_t)t.tv_usec/1000);
        } else {
                return 0;
        }
}

/**
 * Creates an output file and writes the CS_Ipfix_file_header
 */
void IpfixCsExporter::writeFileHeader()
{
	//new Timeouts:
	addToCurTime(&nextChunkTimeout, maxChunkBufferTime*1000);
        addToCurTime(&nextFileTimeout, maxFileCreationInterval*1000);

	std::ofstream out("filename");
	out.write(CS_IPFIX_MAGIC, sizeof(CS_IPFIX_MAGIC));
}

/**
 * Writes the content of chunkList to the output file
 */
void IpfixCsExporter::writeChunkList()
{
        Ipfix_basic_flow_sequence_chunk_header* csChunkHeader = new Ipfix_basic_flow_sequence_chunk_header();	

	csChunkHeader->ipfix_type=0x08;
        csChunkHeader->chunk_length=chunkList.size()*sizeof(struct Ipfix_basic_flow);
        csChunkHeader->flow_count=chunkList.size();

	std::ofstream out("filename");
        out.write(reinterpret_cast<char*>(&csChunkHeader), sizeof(csChunkHeader));

	int i;
	for(i=0; i<chunkList.size(); i++){
		out.write(reinterpret_cast<char*>(&chunkList.front()), sizeof(Ipfix_basic_flow));
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
        if(nextChunkTimeout.tv_sec <= nextFileTimeout.tv_sec){
                //Register a chunk timeout
		timer->addTimeout(this, nextChunkTimeout, NULL);
        }
        else{
                //register a file timeout
                timer->addTimeout(this, nextFileTimeout, NULL);
        }

        timeoutRegistered = true;
}
