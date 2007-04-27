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

#include <list>
#include <pthread.h>
#include <stdint.h>
#include "FlowSink.hpp"

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
class IpfixParser {
	public:
		IpfixParser();
		~IpfixParser();

		/**
		 * Add a FlowSink that receives flows we collect
		 */
		void addFlowSink(FlowSink* flowSink);
		int processMessage(uint8_t* message, uint16_t length, IpfixRecord::SourceID* sourceId);

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

		typedef std::list<FlowSink*> FlowSinks;
		FlowSinks flowSinks; /**< List of FlowSink objects that receive flows we collect */

		friend class TemplateBuffer;
		TemplateBuffer* templateBuffer; /**< TemplateBuffer* structure */

		void processDataSet(IpfixRecord::SourceID* sourceID, IpfixSetHeader* set);
		void processTemplateSet(IpfixRecord::SourceID* sourceID, IpfixSetHeader* set);
		void processDataTemplateSet(IpfixRecord::SourceID* sourceID, IpfixSetHeader* set);
		void processOptionsTemplateSet(IpfixRecord::SourceID* sourceId, IpfixSetHeader* set);
		int processNetflowV9Packet(uint8_t* message, uint16_t length, IpfixRecord::SourceID* sourceId);
		int processIpfixPacket(uint8_t* message, uint16_t length, IpfixRecord::SourceID* sourceId);

};

void printFieldData(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* pattern);

IpfixRecord::FieldInfo* getTemplateFieldInfo(IpfixRecord::TemplateInfo* ti, IpfixRecord::FieldInfo::Type* type);
IpfixRecord::FieldInfo* getTemplateFieldInfo(IpfixRecord::TemplateInfo* ti, IpfixRecord::FieldInfo::Type::Id fieldTypeId, IpfixRecord::FieldInfo::Type::EnterpriseNo fieldTypeEid);
IpfixRecord::FieldInfo* getDataTemplateFieldInfo(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::FieldInfo::Type* type);
IpfixRecord::FieldInfo* getDataTemplateFieldInfo(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::FieldInfo::Type::Id fieldTypeId, IpfixRecord::FieldInfo::Type::EnterpriseNo fieldTypeEid);
IpfixRecord::FieldInfo* getDataTemplateDataInfo(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::FieldInfo::Type* type);
IpfixRecord::FieldInfo* getDataTemplateDataInfo(IpfixRecord::DataTemplateInfo* ti, IpfixRecord::FieldInfo::Type::Id fieldTypeId, IpfixRecord::FieldInfo::Type::EnterpriseNo fieldTypeEid);

#endif
