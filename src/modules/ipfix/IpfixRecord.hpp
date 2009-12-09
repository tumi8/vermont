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
	IeId oppositeDirectionIeId(const IeInfo& type);
	const Packet::IPProtocolType getValidProtocols(const IeInfo& type);
}


/**
 * represents one one of several IPFIX Records, e.g. a Data Record, an Options Template Record, ...
 */
class IpfixRecord
{
	public:
		typedef uint8_t Data;


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

		IpfixRecord() {}
		virtual ~IpfixRecord() {}

		/**
		 * all subclasses *MUST* inherit ManagedInstance, which implements these methods
		 */
		virtual void removeReference() = 0;
		virtual void addReference(int count = 1) = 0;
};


/**
 * Template description passed to the callback function when a new Template arrives.
 */
class TemplateInfo {
        friend class IpfixSender;
	public:
		typedef uint16_t TemplateId;

		enum SetId {
			UnknownSetId = -1,
			NetflowTemplate = 0,
			NetflowOptionsTemplate = 1,
			IpfixTemplate = 2,
			IpfixOptionsTemplate = 3,
			IpfixDataTemplate = 4		
		};

		/**
		 * Information describing a single field in the Template
		 */
		struct FieldInfo {
			InformationElement::IeInfo type;
			int32_t offset; 	/**< offset in bytes from a data start pointer. For internal purposes 0xFFFFFFFF is defined as yet unknown */
			int32_t privDataOffset; /**< offset in bytes from data start pointer for internal private data which is not exported via IPFIX */
			bool isVariableLength; 	/**< true if this field's length might change from record to record, false otherwise */
		};

		TemplateInfo();
		TemplateInfo(const TemplateInfo& t);
		~TemplateInfo();

		void setUniqueId();

		FieldInfo* getFieldInfo(const InformationElement::IeInfo& type);
		FieldInfo* getFieldInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid);
		int getFieldIndex(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid);
		FieldInfo* getDataInfo(const InformationElement::IeInfo& type);
		FieldInfo* getDataInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid);

		TemplateId templateId;	/**< the template id assigned to this template */
		SetId  setId; 		/**< set Id */
		uint16_t fieldCount; 		/**< number of regular fields */
		FieldInfo* fieldInfo; 		/**< array of FieldInfos describing each of these fields */
		
		bool freePointers;  /** small helper variable to indicate if pointers should be freed on destruction */

		// only used by Options Templates:
		uint16_t scopeCount; 		/**< number of scope fields */
		FieldInfo* scopeInfo; 		/**< array of FieldInfos describing each of these fields */

		// only used by Data Templates:
		uint16_t dataCount; 		/**< number of fixed-value fields */
		FieldInfo* dataInfo; 		/**< array of FieldInfos describing each of these fields */
		uint16_t preceding; /**< the preceding rule field as defined in the draft */
		uint16_t dataLength;
		IpfixRecord::Data* data; /**< data start pointer for fixed-value fields */
		bool anonymized; 		/** flag that indicates if fixed-value fields have been anonymized */

	private:
		Mutex mutex; /**< protect access to uniqueIdUseCount */

		/* uniqueId:
		 * - uniqueId>0 is a Vermont-wide unique identifier for a Template
		 * - uniqueId==0 means that no uniqueId has been assigned to this TemplateInfo object, yet
		 * - uniqueId is used by IpfixSender to recognize different Templates quickly (Template ID is
		 *   not sufficient as it is not Vermont-wide unique)
		 * - uniqueId remains unchanged if a TemplateInfo object is copied to change 
		 *   field lengths in the case of variable length fields (for IpfixSender, it still is the same Template)
		 */ 
		uint16_t uniqueId;
		/* uniqueIdUseCount:
		 * - at position i of the vector, we store the number of TemplateInfo objects with uniqueId==(i+1)
		 * - if a new uniqueId is to be assigned, we look for the smallest index i with uniqueIdUseCount[i]==0
		 *   and assigne (i+1) as new uniqueId
		 */ 
		static std::vector<uint16_t> uniqueIdUseCount;
};



class IpfixTemplateRecord : public IpfixRecord, public ManagedInstance<IpfixTemplateRecord> {
	public:
		IpfixTemplateRecord(InstanceManager<IpfixTemplateRecord>* im) : ManagedInstance<IpfixTemplateRecord>(im) { }

		boost::shared_ptr<TemplateInfo> templateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixTemplateRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixTemplateRecord>::addReference(count); }
};

class IpfixDataRecord : public IpfixRecord, public ManagedInstance<IpfixDataRecord> {
	public:
		IpfixDataRecord(InstanceManager<IpfixDataRecord>* im) : ManagedInstance<IpfixDataRecord>(im) {}

		boost::shared_ptr<TemplateInfo> templateInfo;
		int dataLength;
		boost::shared_array<IpfixRecord::Data> message; /**< data block that contains @c data */
		IpfixRecord::Data* data; /**< pointer to start of field data in @c message. Undefined after @c message goes out of scope. */

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixDataRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixDataRecord>::addReference(count); }
};

class IpfixTemplateDestructionRecord : public IpfixRecord, public ManagedInstance<IpfixTemplateDestructionRecord> {
	public:
		IpfixTemplateDestructionRecord(InstanceManager<IpfixTemplateDestructionRecord>* im) : ManagedInstance<IpfixTemplateDestructionRecord>(im) {}
		boost::shared_ptr<TemplateInfo> templateInfo;

		// redirector to reference remover of ManagedInstance
		virtual void removeReference() { ManagedInstance<IpfixTemplateDestructionRecord>::removeReference(); }
		virtual void addReference(int count = 1) { ManagedInstance<IpfixTemplateDestructionRecord>::addReference(count); }
};




#endif

