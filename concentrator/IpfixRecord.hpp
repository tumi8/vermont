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

#ifndef INCLUDE_IpfixRecord_hpp
#define INCLUDE_IpfixRecord_hpp

#include <stdint.h>
#include <memory>
#include <boost/smart_ptr.hpp>
#include <stdexcept>
#include "../sampler/Lock.h"

#define MAX_ADDRESS_LEN 16

typedef uint16_t TemplateID;

/**
 * represents one one of several IPFIX Records, e.g. a Data Record, an Options Template Record, ...
 */
class IpfixRecord {
	public:
		typedef uint8_t Data;

		/**
		 * Information describing a single field in the fields passed via various callback functions.
		 */
		struct FieldInfo {
			/**
			 * IPFIX field type and length.
			 * if "id" is < 0x8000, i.e. no user-defined type, "eid" is 0
			 */ 
			struct Type {

				typedef uint16_t Id;
				typedef uint16_t Length;
				typedef uint32_t EnterpriseNo;

				IpfixRecord::FieldInfo::Type::Id id; /**< type tag of this field, according to [INFO] */
				IpfixRecord::FieldInfo::Type::Length length; /**< length in bytes of this field */
				int isVariableLength; /**< true if this field's length might change from record to record, false otherwise */
				IpfixRecord::FieldInfo::Type::EnterpriseNo eid; /**< enterpriseNo for user-defined data types (i.e. type >= 0x8000) */	
			};

			IpfixRecord::FieldInfo::Type type;
			uint16_t offset; /**< offset in bytes from a data start pointer. For internal purposes 65535 is defined as yet unknown */
		};


		/**
		 * Template description passed to the callback function when a new Template arrives.
		 */
		struct TemplateInfo {
			// FIXME: Add destructor for fieldInfo
			uint16_t templateId; /**< the template id assigned to this template or 0 if we don't know or don't care */
			uint16_t fieldCount; /**< number of regular fields */
			IpfixRecord::FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
			void* userData; /**< pointer to a field that can be used by higher-level modules */
		};

		/**
		 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
		 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
		 */
		struct OptionsTemplateInfo {
			// FIXME: Add destructor for fieldInfo
			// FIXME: Add destructor for scopeInfo
			uint16_t templateId; /**< the template id assigned to this template or 0 if we don't know or don't care */
			uint16_t scopeCount; /**< number of scope fields */
			IpfixRecord::FieldInfo* scopeInfo; /**< array of FieldInfos describing each of these fields */
			uint16_t fieldCount; /**< number of regular fields. This is NOT the number of all fields */
			IpfixRecord::FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
			void* userData; /**< pointer to a field that can be used by higher-level modules */
		};

		/**
		 * DataTemplate description passed to the callback function when a new DataTemplate arrives.
		 */
		struct DataTemplateInfo {
			// FIXME: Add destructor for fieldInfo
			// FIXME: Add destructor for dataInfo
			// FIXME: Add destructor for data
			uint16_t id; /**< the template id assigned to this template or 0 if we don't know or don't care */
			uint16_t preceding; /**< the preceding rule field as defined in the draft */
			uint16_t fieldCount; /**< number of regular fields */
			IpfixRecord::FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
			uint16_t dataCount; /**< number of fixed-value fields */
			IpfixRecord::FieldInfo* dataInfo; /**< array of FieldInfos describing each of these fields */
			IpfixRecord::Data* data; /**< data start pointer for fixed-value fields */
			void* userData; /**< pointer to a field that can be used by higher-level modules */
		};

		struct SourceID {

			struct ExporterAddress {
				char ip[MAX_ADDRESS_LEN];
				uint8_t len;
			};

			uint32_t observationDomainId;
			SourceID::ExporterAddress exporterAddress;
		};

		boost::shared_ptr<IpfixRecord::SourceID> sourceID;

		virtual ~IpfixRecord();
};

class IpfixTemplateRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;
};

class IpfixOptionsTemplateRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;
};

class IpfixDataTemplateRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;
};

class IpfixDataRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */
};

class IpfixOptionsRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */
};

class IpfixDataDataRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */
};

class IpfixTemplateDestructionRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;
};

class IpfixOptionsTemplateDestructionRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;
};

class IpfixDataTemplateDestructionRecord : public IpfixRecord {
	public:
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;
};

#endif

