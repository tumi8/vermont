/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
 *               2009 Gerhard Muenz
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

#ifndef INCLUDED_IpfixParser_hpp
#define INCLUDED_IpfixParser_hpp

#include "IpfixReceiver.hpp"
#include "IpfixRecordSender.h"

#include <pthread.h>
#include <stdint.h>
#include <boost/smart_ptr.hpp>
#include <map>


class TemplateBuffer;

/**
 * IPFIX Parser module.
 *
 * The IPFIX Parser module receives messages from lower levels (see @c processMessage())
 * and parses the message into separate Templates, Options and Flows. It then
 * invokes the appropriate callback routine for each Template, Option and Flow received
 * (see the @c setTemplateCallback() and @c setDataRecordCallback() function groups).
 *
 * The Collector module supports higher-level modules by providing field types and offsets along 
 * with the raw data block of individual messages passed via the callback functions (see @c TemplateInfo)
 */
class IpfixParser : public IpfixPacketProcessor, public Sensor 
{
	public:
		IpfixParser(IpfixRecordSender* sender);
		virtual ~IpfixParser();

		virtual int processPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId);
		virtual std::string getStatisticsXML(double interval); 
		
		virtual void performStart();
		virtual void performShutdown();
		virtual void preReconfiguration();
		virtual void onReconfiguration1();
		virtual void postReconfiguration();

		void setTemplateLifetime(uint16_t time)
		{
			templateLifetime = time;
		}

		/**
		 * IPFIX header helper.
		 * Constitutes the first 16 bytes of every IPFIX Message
		 */
		typedef struct {
			uint16_t version; /**< Expected to be 0x000a */
			uint16_t length; 
			uint32_t exportTime;
			uint32_t sequenceNo;
			uint32_t observationDomainId;
			uint8_t data;
		} IpfixHeader;
	protected:

		/**
		 * NetflowV9 header helper.
		 * Constitutes the first bytes of every NetflowV9 Message
		 */
		typedef struct {
			uint16_t version; /**< Expected to be 0x0009 */
			uint16_t setCount;
			uint32_t upTime;
			uint32_t exportTime;
			uint32_t sequenceNo;
			uint32_t observationDomainId;
			uint8_t data;
		} NetflowV9Header;

		/**
		 * IPFIX "Set" helper.
		 * Constitutes the first bytes of every IPFIX Template Set, Options Template Set or Data Set
		 */
		typedef struct {
			uint16_t id;
			uint16_t length;
			uint8_t data; 
		} IpfixSetHeader;

		/**
		 * IPFIX "Template Set" helper.
		 * Constitutes the first bytes of every IPFIX Template
		 */
		typedef struct {
			uint16_t templateId;
			uint16_t fieldCount;
			uint8_t data;
		} IpfixTemplateHeader;

		/**
		 * IPFIX "DataTemplate Set" helper.
		 * Constitutes the first bytes of every IPFIX DataTemplate
		 */
		typedef struct {
			uint16_t templateId;
			uint16_t fieldCount;
			uint16_t dataCount;
			uint16_t precedingRule;
			uint8_t data;
		} IpfixDataTemplateHeader;

		/**
		 * IPFIX "Options Template Set" helper.
		 * Constitutes the first bytes of every IPFIX Options Template
		 */
		typedef struct {
			uint16_t templateId;
			uint16_t fieldCount;
			uint16_t scopeCount; 
			uint8_t data;
		} IpfixOptionsTemplateHeader;

		friend class TemplateBuffer;
		TemplateBuffer* templateBuffer; /**< TemplateBuffer* structure */

		uint16_t templateLifetime;

		pthread_mutex_t mutex; /**< Used to give only one IpfixReceiver access to the IpfixPacketProcessor */

		uint32_t processDataSet(boost::shared_ptr<IpfixRecord::SourceID> sourceID, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		uint32_t processTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceID, TemplateInfo::SetId setId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		uint32_t processDataTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceID, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		uint32_t processOptionsTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::SetId setId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		int processNetflowV9Packet(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId);
		int processIpfixPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId);
		
		virtual void push(IpfixRecord* ipfixRecord);

	private:
		bool processStructuredData(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data);
		bool processStructuredDataBasicList(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data, IpfixRecord::Data * const endOfData);
		bool processStructuredDataSubTemplateList(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data, IpfixRecord::Data *endOfData);
		bool processStructuredDataSetSubTemplateMultiList(boost::shared_ptr<IpfixRecord::SourceID> sourceId, TemplateInfo::FieldInfo *field, IpfixRecord::Data *data, IpfixRecord::Data * const endOfData);

		uint64_t statTotalDataRecords; /**< number of data records processed by parser */
		uint64_t statTotalTemplateRecords; /**< number of template records processed by parser */
		uint64_t statTotalMessages; /**< number of IPFIX/Netflow messages successfully processed by parser */
		IpfixRecordSender* ipfixRecordSender;
		
		static InstanceManager<IpfixTemplateRecord> templateRecordIM;
		static InstanceManager<IpfixDataRecord> dataRecordIM;
		static InstanceManager<IpfixTemplateDestructionRecord> templateDestructionRecordIM;
		
		void resendBufferedTemplates();
		void withdrawBufferedTemplates();

		// TODO: extend Sequence Number check for 
		//       - reorder out-of-order packets
		//       - SCTP (therefore, we need to consider the SCTP stream id)
		//       - expire this information? 
		//         at the moment, it is stored forever, even if exporter has died (which we cannot detect)
		struct SNInfo {
			SNInfo() : expectedSN(0), 
				receivedMessages(0), receivedDataRecords(0), receivedTemplateRecords(0),
				outOfOrderMessages(0), lostMessages(0), lostDataRecords(0) {}

			uint32_t expectedSN;
			uint32_t receivedMessages;
			uint32_t receivedDataRecords;
			uint32_t receivedTemplateRecords;
			uint32_t outOfOrderMessages;
			// Used in case of NetflowV9:
			uint32_t lostMessages;
			// Used in case of IPFIX:
			uint32_t lostDataRecords;
		};
		
		std::map<IpfixRecord::SourceID, SNInfo> snInfoMap;
};

#endif
