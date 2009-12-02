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

#include "common/ipfixlolib/ipfix.h"
#include "modules/packet/Packet.h"

#define MAX_ADDRESS_LEN 16

namespace InformationElement {

	typedef uint16_t IeId;
	typedef uint16_t IeLength;
	typedef uint32_t IeEnterpriseNumber;

	/* Field in a Data Record */
	struct IeInfo {
		IeId id; 			/**< Information Element Id */
		IeEnterpriseNumber enterprise;/**< Enterprise Number for enterprise-specific IEs (i.e., id >= 0x8000) */
		IeLength length; 		/**< Field length in bytes (65535 in the case of variable lengh field) */
	};

	IeLength getFieldLength(IeId id);
	bool isBiflowField(const IeInfo& type);
	const Packet::IPProtocolType getValidProtocols(const IeInfo& type);
}

typedef uint16_t TemplateID;

/**
 * represents one one of several IPFIX Records, e.g. a Data Record, an Options Template Record, ...
 */
class IpfixRecord
{
	public:
		typedef uint8_t Data;

		/**
		 * Information describing a single field in the fields passed via various callback functions.
		 */
		struct FieldInfo {

			InformationElement::IeInfo type;
			int32_t offset; /**< offset in bytes from a data start pointer. For internal purposes 0xFFFFFFFF is defined as yet unknown */
			int32_t privDataOffset; /**< offset in bytes from data start pointer for internal private data which is not exported via IPFIX */
			bool isVariableLength; 	/**< true if this field's length might change from record to record, false otherwise */
		};


		/**
		 * Template description passed to the callback function when a new Template arrives.
		 */
		struct TemplateInfo {
			TemplateInfo() : fieldInfo(NULL), destroyed(false), freePointers(true) {	}

			TemplateInfo(const TemplateInfo& t)
			{
				templateId = t.templateId;
				fieldCount = t.fieldCount; /**< number of regular fields */
				fieldInfo = (IpfixRecord::FieldInfo*)malloc(fieldCount*sizeof(FieldInfo));
				memcpy(fieldInfo, t.fieldInfo, fieldCount*sizeof(FieldInfo));
				userData = t.userData;
				destroyed = t.destroyed;
				freePointers = t.freePointers;
			}

			~TemplateInfo() {
				if (freePointers) free(fieldInfo);
			}

			/**
			 * Gets a Template's FieldInfo by field id. Length is ignored.
			 * @param type Field id and enterprise to look for. Length is ignored.
			 * @return NULL if not found
			 */
			IpfixRecord::FieldInfo* getFieldInfo(InformationElement::IeInfo* type) {
				return getFieldInfo(type->id, type->enterprise);
			}

//			IpfixRecord::Data* getFieldPointer(InformationElement::IeInfo* type, IpfixRecord::Data* pdata) {
//				return getFieldPointer(type->id, &pdata);
//			}
			/**
			 * Gets a Template's FieldInfo by field id. Length is ignored.
			 * @param fieldTypeId FieldInfo::Type id to look for
			 * @param fieldTypeEid FieldInfo::Type enterprise to look for
			 * @return NULL if not found
			 */
			IpfixRecord::FieldInfo* getFieldInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid) {
				int i;

				for (i = 0; i < fieldCount; i++) {
					if ((fieldInfo[i].type.id == fieldTypeId) && (fieldInfo[i].type.enterprise == fieldTypeEid)) {
						return &fieldInfo[i];
					}
				}

				return NULL;
			}

			int getFieldIndex(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid) {
				int i;

				for (i = 0; i < fieldCount; i++) {
					if ((fieldInfo[i].type.id == fieldTypeId) && (fieldInfo[i].type.enterprise == fieldTypeEid)) {
						return i;
					}
				}

				return -1;
			}


			uint16_t templateId; /**< the template id assigned to this template or 0 if we don't know or don't care */
			uint16_t fieldCount; /**< number of regular fields */
			IpfixRecord::FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
			void* userData; /**< pointer to a field that can be used by higher-level modules */

			/**
			 * if this template is to be destroyed because of module reconfiguration, this flag is set to true
			 * it should be checked in every module which caches this structure and be destroyed in method
			 * Module::preRegistration2()
			 **/
			bool destroyed;

			bool freePointers;  /** small helper variable to indicate if pointers should be freed on destruction */
		};

		/**
		 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
		 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
		 */
		struct OptionsTemplateInfo {
			OptionsTemplateInfo() : scopeInfo(NULL), fieldInfo(NULL) {
			}

			OptionsTemplateInfo(const OptionsTemplateInfo& t)
			{
				templateId = t.templateId;

				scopeCount = t.scopeCount;
				scopeInfo = (FieldInfo*)malloc(scopeCount*sizeof(FieldInfo));
				memcpy(scopeInfo, t.scopeInfo, scopeCount*sizeof(FieldInfo));

				fieldCount = t.fieldCount; /**< number of regular fields */
				fieldInfo = (FieldInfo*)malloc(fieldCount*sizeof(FieldInfo));
				memcpy(fieldInfo, t.fieldInfo, fieldCount*sizeof(FieldInfo));

				userData = t.userData;
			}

			~OptionsTemplateInfo() {
				free(fieldInfo);
				free(scopeInfo);
			}

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
		struct DataTemplateInfo : public TemplateInfo
		{
			DataTemplateInfo() : TemplateInfo(), preceding(0), dataCount(0), dataInfo(NULL), dataLength(0), data(NULL), anonymized (false) {
			}

			DataTemplateInfo(const DataTemplateInfo& t)
			{
				templateId = t.templateId;
				preceding = t.preceding;

				fieldCount = t.fieldCount;
				fieldInfo = (FieldInfo*)malloc(fieldCount*sizeof(FieldInfo));
				memcpy(fieldInfo, t.fieldInfo, fieldCount*sizeof(FieldInfo));

				dataCount = t.dataCount;
				dataInfo = (FieldInfo*)malloc(dataCount*sizeof(FieldInfo));
				memcpy(dataInfo, t.dataInfo, dataCount*sizeof(FieldInfo));

				dataLength = t.dataLength;
				data = (Data*)malloc(dataLength*sizeof(Data));
				memcpy(data, t.data, dataLength*sizeof(Data));
				anonymized = t.anonymized;

				userData = t.userData;
				destroyed = t.destroyed;
				freePointers = t.freePointers;
			}

			~DataTemplateInfo() {
				if (freePointers) {
				    free(fieldInfo);
				    free(dataInfo);
				    free(data);
				    freePointers = false;
				}
			}


			IpfixRecord::FieldInfo* getDataInfo(InformationElement::IeInfo* type) {
				return getDataInfo(type->id, type->enterprise);
			}

			/**
			 * Gets a DataTemplate's Data-FieldInfo by field id.
			 * @param fieldTypeId Field id to look for
			 * @param fieldTypeEid Field enterprise to look for
			 * @return NULL if not found
			 */
			IpfixRecord::FieldInfo* getDataInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid) {
				int i;

				for (i = 0; i < dataCount; i++) {
					if ((dataInfo[i].type.id == fieldTypeId) && (dataInfo[i].type.enterprise == fieldTypeEid)) {
						return &dataInfo[i];
					}
				}

				return NULL;
			}

			uint16_t preceding; /**< the preceding rule field as defined in the draft */
			uint16_t dataCount; /**< number of fixed-value fields */
			IpfixRecord::FieldInfo* dataInfo; /**< array of FieldInfos describing each of these fields */
			uint16_t dataLength;
			IpfixRecord::Data* data; /**< data start pointer for fixed-value fields */
			bool anonymized; /** flag that indicates if fixed-value fields have been anonymized */
			void* userData; /**< pointer to a field that can be used by higher-level modules */
		};

		/* This struct is called SourceID for historic reasons.
		 * A better name would be something like TemplateScope (TransportSession + ObservationDomainId)
		 */
		struct SourceID {

			struct ExporterAddress {
				uint8_t ip[MAX_ADDRESS_LEN];
				uint8_t len;
			};

			uint32_t observationDomainId;
			SourceID::ExporterAddress exporterAddress;
			uint16_t exporterPort;
			uint16_t receiverPort;
			uint8_t protocol;
			int fileDescriptor;

			bool operator==(const struct SourceID & x) const {
				if(protocol == 132) /* compare file descriptors instead of IP addresses because of possible multihoming */
					return (observationDomainId == x.observationDomainId) &&
// 					(exporterPort == x.exporterPort) &&
// 					(receiverPort == x.receiverPort) &&
// 					(protocol == x.protocol) &&
					(fileDescriptor == x.fileDescriptor);
				else /* UDP: fileDescriptor only specifies the Collector endpoint*/
					return (observationDomainId == x.observationDomainId) &&
					(exporterPort == x.exporterPort) &&
					//(receiverPort == x.receiverPort) &&
					(fileDescriptor == x.fileDescriptor) &&
					//(protocol == x.protocol) &&
					(exporterAddress.len == x.exporterAddress.len) &&
					(memcmp(exporterAddress.ip, x.exporterAddress.ip, exporterAddress.len) == 0 );

			}
			// compare two SourceIDs without considering Observation Domain ID
			bool equalIgnoringODID(const struct SourceID & x) const {
				if(protocol == 132) /* compare file descriptors instead of IP addresses because of possible multihoming */
					return (fileDescriptor == x.fileDescriptor);
				else /* UDP: fileDescriptor only specifies the Collector endpoint*/
					return (exporterPort == x.exporterPort) &&
					//(receiverPort == x.receiverPort) &&
					(fileDescriptor == x.fileDescriptor) &&
					//(protocol == x.protocol) &&
					(exporterAddress.len == x.exporterAddress.len) &&
					(memcmp(exporterAddress.ip, x.exporterAddress.ip, exporterAddress.len) == 0 );
			}

		};

		boost::shared_ptr<IpfixRecord::SourceID> sourceID;

		IpfixRecord();
		virtual ~IpfixRecord();

		/**
		 * all subclasses *MUST* inherit ManagedInstance, which implements these methods
		 */
		virtual void removeReference() = 0;
		virtual void addReference(int count = 1) = 0;
};

class IpfixTemplateRecord : public IpfixRecord, public ManagedInstance<IpfixTemplateRecord> {
	public:
		IpfixTemplateRecord(InstanceManager<IpfixTemplateRecord>* im);
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixTemplateRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixTemplateRecord>::addReference(count); }
};

class IpfixOptionsTemplateRecord : public IpfixRecord, public ManagedInstance<IpfixOptionsTemplateRecord> {
	public:
		IpfixOptionsTemplateRecord(InstanceManager<IpfixOptionsTemplateRecord>* im);
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixOptionsTemplateRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixOptionsTemplateRecord>::addReference(count); }
};

class IpfixDataTemplateRecord : public IpfixRecord, public ManagedInstance<IpfixDataTemplateRecord> {
	public:
		IpfixDataTemplateRecord(InstanceManager<IpfixDataTemplateRecord>* im);
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixDataTemplateRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixDataTemplateRecord>::addReference(count); }
};

class IpfixDataRecord : public IpfixRecord, public ManagedInstance<IpfixDataRecord> {
	public:
		IpfixDataRecord(InstanceManager<IpfixDataRecord>* im);
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixDataRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixDataRecord>::addReference(count); }
};

class IpfixOptionsRecord : public IpfixRecord, public ManagedInstance<IpfixOptionsRecord> {
	public:
		IpfixOptionsRecord(InstanceManager<IpfixOptionsRecord>* im);
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixOptionsRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixOptionsRecord>::addReference(count); }
};

class IpfixDataDataRecord : public IpfixRecord, public ManagedInstance<IpfixDataDataRecord>
{
	public:
		IpfixDataDataRecord(InstanceManager<IpfixDataDataRecord>* im);
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixDataDataRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixDataDataRecord>::addReference(count); }
};

class IpfixTemplateDestructionRecord : public IpfixRecord, public ManagedInstance<IpfixTemplateDestructionRecord> {
	public:
		IpfixTemplateDestructionRecord(InstanceManager<IpfixTemplateDestructionRecord>* im);
		boost::shared_ptr<IpfixRecord::TemplateInfo> templateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixTemplateDestructionRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixTemplateDestructionRecord>::addReference(count); }
};

class IpfixOptionsTemplateDestructionRecord : public IpfixRecord, public ManagedInstance<IpfixOptionsTemplateDestructionRecord> {
	public:
		IpfixOptionsTemplateDestructionRecord(InstanceManager<IpfixOptionsTemplateDestructionRecord>* im);
		boost::shared_ptr<IpfixRecord::OptionsTemplateInfo> optionsTemplateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixOptionsTemplateDestructionRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixOptionsTemplateDestructionRecord>::addReference(count); }
};

class IpfixDataTemplateDestructionRecord : public IpfixRecord, public ManagedInstance<IpfixDataTemplateDestructionRecord> {
	public:
		IpfixDataTemplateDestructionRecord(InstanceManager<IpfixDataTemplateDestructionRecord>* im);
		boost::shared_ptr<IpfixRecord::DataTemplateInfo> dataTemplateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixDataTemplateDestructionRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixDataTemplateDestructionRecord>::addReference(count); }
};

#endif

