/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007 Gerhard Muenz
 * Copyright (C) 2008 Tobias Limmer
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


#ifndef IPFIXCSEXPORTER_H
#define IPFIXCSEXPORTER_H

#include "IpfixRecord.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include "core/Source.h"
#include "core/Notifiable.h"
#include "modules/ipfix/Connection.h"
#include "modules/ipfix/IpfixRecordDestination.h"
#include <netinet/in.h>
#include <time.h>
#include <iostream>
#include <fstream>
#include <list>

#define EXPORTERID 0
#define DEFAULTFILESIZE 2097152

class IpfixCsExporter : public Module, public Source<NullEmitable*>, public IpfixRecordDestination, public Notifiable
{
	public:
		IpfixCsExporter(std::string filenamePrefix,
		std::string destinationPath, uint32_t maxFileSize,
		uint32_t maxChunkBufferTime, uint32_t maxChunkBufferRecords,
		uint32_t maxFileCreationInterval, uint8_t exportMode);

		virtual ~IpfixCsExporter();

        	virtual void onDataRecord(IpfixDataRecord* record);
	        virtual void onTimeout(void* dataPtr);

	protected:
		virtual void performStart();
		virtual void performShutdown();

	private:
		//Some predefined structs
		const static uint8_t CS_IPFIX_CHUNK_TYPE = 0x08;
		char CS_IPFIX_MAGIC[8];

		std::string filenamePrefix; /**< prefix to each file */
		std::string destinationPath; /**< storage path of output files */
		FILE *currentFile;
		char currentFilename[512];
		char currentTmpname[512];
		uint16_t globalIterator;
		uint32_t maxFileSize; /**< maximum filesize in  KiB, i.e. maximumFilesize * 1024 == maximum filesize in bytes */
		uint32_t maxChunkBufferTime; /**< maximum Chunk Buffer Time (in seconds) */
		uint32_t maxChunkBufferRecords; /**< maximum Chunk Buffer Records (in records) */
		uint32_t maxFileCreationInterval; /**< time in seconds between creation of a new output file */
		uint8_t exportMode; /**< export Mode */
		uint32_t currentFileSize;
		bool timeoutRegistered;
		timespec nextChunkTimeout;
		timespec nextFileTimeout;

		//to calculate criteria after given timeouts
		void registerTimeout();
		uint64_t retrieveTime(IpfixDataRecord* record, InformationElement::IeId id1, InformationElement::IeId id2,
				InformationElement::IeId id3, InformationElement::IeEnterpriseNumber pen);

		//file write operations
		void writeFileHeader();
		void writeChunkList();
		void closeFile();

		enum cs_export_mode {
			CS_E_PLAIN      = 0,
			CS_E_ANON       = 1,
			CS_E_PSEUDO     = 2,
		};

		struct CS_Ipfix_file_header {
			uint8_t magic[8]; //expected: CS_IPFIX_MAGIC
		} DISABLE_ALIGNMENT

		/**
		 * class representing the start of a chunk in the CS format
		 */
		struct Ipfix_basic_flow_sequence_chunk_header {
			uint16_t ipfix_type; //-> CS_IPFIX_CHUNK_TYPE -> 0x0008
			uint32_t chunk_length; //number of bytes in chunk starting after this element, this should be flow_count*sizeof(Ipfix_basic_flow)+4
			uint32_t flow_count; //number of Ipfix_basic_flow records to follow
		} DISABLE_ALIGNMENT

		struct Ipfix_basic_flow {
			uint16_t record_length;                 // total length of this record minus length of this element
			uint8_t  src_export_mode;               // expected to match enum cs_export_mode
			uint8_t  dst_export_mode;               // expected to match enum cs_export_mode
			uint8_t  ipversion;                     // expected 4 (for now)
			uint32_t source_ipv4_address;
			uint32_t destination_ipv4_address;
			uint8_t  protocol_identifier;
			uint16_t source_transport_port;         // encode udp/tcp ports here
			uint16_t destination_transport_port;    // encode udp/tcp ports here
			uint8_t  icmp_type_ipv4;
			uint8_t  icmp_code_ipv4;
			uint8_t  tcp_control_bits;
			uint64_t flow_start_milliseconds;       // encode flowStart(Micro|Nano|)Seconds here
			uint64_t flow_end_milliseconds;         // encode flowEnd(Micro|Nano|)Seconds here
			uint64_t octet_total_count;
			uint64_t packet_total_count;
			uint8_t  biflow_direction;
			uint64_t rev_octet_total_count;
			uint64_t rev_packet_total_count;
			uint8_t  rev_tcp_control_bits;
		} DISABLE_ALIGNMENT

		list<Ipfix_basic_flow*> chunkList;
		uint32_t chunkListSize;
};


#endif
