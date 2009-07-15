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

#ifndef INCLUDED_IpfixParser_hpp
#define INCLUDED_IpfixParser_hpp

#define NetflowV9_SetId_Template  0

#include "IpfixReceiver.hpp"
#include "IpfixRecordSender.h"

#include <list>
#include <pthread.h>
#include <stdint.h>
#include <boost/smart_ptr.hpp>


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
 * with the raw data block of individual messages passed via the callback functions (see @c IpfixRecord::TemplateInfo)
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
		virtual void onReconfiguration1();
		virtual void postReconfiguration();

		void setTemplateLivetime(uint16_t time);

	protected:
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

		/**
		 * NetflowV9 header helper.
		 * Constitutes the first bytes of every NetflowV9 Message
		 */
		typedef struct {
			uint16_t version; /**< Expected to be 0x0009 */
			uint16_t setCount;
			uint32_t uptime;
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

		uint16_t templateLivetime;

		pthread_mutex_t mutex; /**< Used to give only one IpfixReceiver access to the IpfixPacketProcessor */

		uint32_t processDataSet(boost::shared_ptr<IpfixRecord::SourceID> sourceID, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		void processTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceID, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		void processOptionsTemplateSet(boost::shared_ptr<IpfixRecord::SourceID> sourceId, boost::shared_array<uint8_t> message, IpfixSetHeader* set, uint8_t* endOfMessage);
		int processNetflowV9Packet(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId);
		int processIpfixPacket(boost::shared_array<uint8_t> message, uint16_t length, boost::shared_ptr<IpfixRecord::SourceID> sourceId);
		
		virtual void push(IpfixRecord* ipfixRecord);


	private:
		uint32_t statTotalDataRecords; /**< amount of data records processed by parser */
		uint32_t statTotalDRPackets; /**< amount of UDP packets containing data records processed by parser */
		IpfixRecordSender* ipfixRecordSender;
		
		static InstanceManager<IpfixTemplateRecord> templateRecordIM;
		static InstanceManager<IpfixOptionsTemplateRecord> optionsTemplateRecordIM;
		static InstanceManager<IpfixDataRecord> dataRecordIM;
		static InstanceManager<IpfixOptionsRecord> optionsRecordIM;
		static InstanceManager<IpfixTemplateDestructionRecord> templateDestructionRecordIM;
		static InstanceManager<IpfixOptionsTemplateDestructionRecord> optionsTemplateDestructionRecordIM;
		
		void resendBufferedTemplates();
		void setTemplateDestroyed(bool destroyed);

};

#endif
