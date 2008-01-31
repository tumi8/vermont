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
#include <vector>

/**
 * IPFIX Exporter interface.
 *
 * Interface for feeding generated Templates and Data Records to "ipfixlolib" 
 */
class IpfixSender : public FlowSink {
	public:
		IpfixSender(uint16_t observationDomainId, const char* ip = 0, uint16_t port = 0, ipfix_transport_protocol proto = UDP);
		virtual ~IpfixSender();

		void start();
		void stop();

		int addCollector(const char *ip, uint16_t port, ipfix_transport_protocol proto);

		int onDataTemplate(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
                int onDataTemplateDestruction(IpfixRecord::SourceID* sourceID, IpfixRecord::DataTemplateInfo* dataTemplateInfo);
                int onDataDataRecord(boost::shared_ptr<IpfixDataDataRecord> rec);
		int onIdle();

	        virtual void flowSinkProcess();

		void stats();
		// Set up time after that Templates are going to be resent
		bool setTemplateTransmissionTimer(uint32_t timer){
			ipfix_set_template_transmission_timer(ipfixExporter, timer);
			
			return true;
		}
		// Set up SCTP packet lifetime
		bool setSctpLifetime(uint32_t time){
			ipfix_set_sctp_lifetime(ipfixExporter, time);
			
			return true;
		}
		// Set up SCTP reconnect timer
		bool setSctpReconnectTimeout(uint32_t time){
			ipfix_set_sctp_reconnect_timer(ipfixExporter, time);
			
			return true;
		}
		
	protected:
		ipfix_exporter* ipfixExporter; /**< underlying ipfix_exporter structure. */
		uint16_t lastTemplateId; /**< Template ID of last created Template */
		uint32_t sentRecords; /**< Statistics: Total number of records sent since last statistics were polled */

	private:
		int startDataSet(uint16_t templateId);
		int endAndSendDataSet();

		std::vector<boost::shared_ptr<IpfixRecord> > recordsToRelease;
		
		uint16_t ringbufferPos; /**< Pointer to next free slot in @c conversionRingbuffer. */
		uint8_t conversionRingbuffer[65536]; /**< Ringbuffer used to store converted imasks between @c ipfix_put_data_field() and @c ipfix_send() */
		uint16_t recordsInDataSet; /**< The number of records in the current data set */
		uint16_t currentTemplateId; /**< Template ID of the unfinished data set */
};

#endif
