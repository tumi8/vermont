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

#ifndef SNDIPFIX_H
#define SNDIPFIX_H

#include "IpfixParser.hpp"
#include "ipfixlolib/ipfixlolib.h"

/**
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib" 
 */
class IpfixSender : public FlowSink {
	public:
		IpfixSender(uint16_t observationDomainId, const char* ip, uint16_t port);
		~IpfixSender();

		void start();
		void stop();

		int addCollector(const char *ip, uint16_t port);

		int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		int onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
		int onDataDataRecord(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo, uint16_t length, IpfixRecord::Data* data);

		void stats();

	protected:
		ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. */
		uint16_t lastTemplateId; /**< Template ID of last created Template */
		char ip[128]; /**< IP of Collector we export to */
		uint16_t port; /**< Port of Collector we export to */
		uint32_t sentRecords; /**< Statistics: Total number of records sent since last statistics were polled */

	private:
		uint16_t ringbufferPos; /**< Pointer to next free slot in @c conversionRingbuffer. */
		uint8_t conversionRingbuffer[65536]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */
};

#endif
