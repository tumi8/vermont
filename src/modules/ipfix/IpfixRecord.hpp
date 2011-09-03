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

#ifndef INCLUDE_IpfixRecord_hpp
#define INCLUDE_IpfixRecord_hpp

#include <stdint.h>
#include <memory>
#include <boost/smart_ptr.hpp>
#include <stdexcept>
#include "common/Misc.h"

#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfix_names.h"
#include "modules/packet/Packet.h"

#define MAX_ADDRESS_LEN 16

namespace InformationElement {

	typedef uint16_t IeId;
	typedef uint16_t IeLength;
	typedef uint32_t IeEnterpriseNumber;

	/* Field in a Data Record */
	class IeInfo {
	public:
		IeInfo(IeId id, IeEnterpriseNumber enterprise, IeLength length = 0)
			: id(id), enterprise(enterprise), length(length)
		{
			if (length==0) {
				const ipfix_identifier* ipfixid = ipfix_id_lookup(id, enterprise);
				if (ipfixid)
					length = ipfixid->length;
				else {
					msg(MSG_INFO, "WARNING: received unknown IE type id: %s", toString().c_str());
				}
			}
		}

		IeInfo()
			: id(0), enterprise(0), length(0)
		{}

		IeInfo(const ipfix_identifier* ipfixid)
			: id(ipfixid->id), enterprise(ipfixid->pen), length(ipfixid->length)
		{
		}

		bool operator==(const IeInfo &other) const {
			return (id==other.id) && (enterprise==other.enterprise);
		}

		bool operator<(const IeInfo &other) const {
			if (enterprise<other.enterprise) return true;
			if (id<other.id) return true;
			return false;
		}


		IeId id; 			/**< Information Element Id */
		IeEnterpriseNumber enterprise;/**< Enterprise Number for enterprise-specific IEs (i.e., id >= 0x8000) */
		IeLength length; 		/**< Field length in bytes (65535 in the case of variable lengh field) */

		bool isReverseField() const;
		IeInfo getReverseDirection();
		const Packet::IPProtocolType getValidProtocols();
		string toString() const;
		bool existsReverseDirection();
		bool isStructuredData() const;
	};




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
			SourceID() : observationDomainId(0), sysUpTime(0), exportTime(0),
			    exporterPort(0), receiverPort(0), protocol(0), fileDescriptor(0)
			{
				exporterAddress.len = 0;
			}

			struct ExporterAddress {
				uint8_t ip[MAX_ADDRESS_LEN];
				uint8_t len;

				// conversion of IPv4 addresses to uint32_t
				uint32_t toUInt32() const
				{
					if (len == 4)
						return (((uint32_t)ip[0])<<24) + (((uint32_t)ip[1])<<16) + (((uint32_t)ip[2])<<8) + (uint32_t)ip[3];
					else
						return 0;
				}
			};

			// fields from IPFIX/Netflow header:
			uint32_t observationDomainId;
			uint32_t sysUpTime; // time in milliseconds elapsed since system started (only available in Netflow header)
			uint32_t exportTime;

			// information from collector:
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
					(fileDescriptor == x.fileDescriptor) && // this check includes receiverPort and protocol
					//(protocol == x.protocol) &&
					(exporterAddress.len == x.exporterAddress.len) &&
					(memcmp(exporterAddress.ip, x.exporterAddress.ip, exporterAddress.len) == 0 );

			}

			// operator < is needed for using SourceID as key in maps
			bool operator<(const struct SourceID & x) const {
				int tmp;
				if(fileDescriptor < x.fileDescriptor) // this check includes receiverPort and protocol
					return true;
				if(fileDescriptor > x.fileDescriptor)
					return false;

				if(exporterAddress.len < x.exporterAddress.len)
					return true;
				if(exporterAddress.len > x.exporterAddress.len)
					return false;

			        if((tmp = memcmp(exporterAddress.ip, x.exporterAddress.ip, exporterAddress.len)) < 0)
					return true;
			        if(tmp > 0)
					return false;

				if(exporterPort < x.exporterPort)
					return true;
				if(exporterPort > x.exporterPort)
					return false;

				if(observationDomainId < x.observationDomainId)
					return true;

				return false;
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

			std::string toString() const {
				ostringstream oss;
				if(exporterAddress.len == 4) {
					oss << IPToString(*(uint32_t*)(&exporterAddress)).c_str();
				} else {
					oss << "non-IPv4 address";
				}
				oss << ":" << exporterPort;
				switch(protocol) {
					case 17:
						oss << " (UDP)";
						break;
					case 132:
						oss << " (SCTP)";
						break;
					default:
						oss << " (" << protocol <<")";
				}
				oss << " ODID=" << observationDomainId;
				return oss.str();
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

		struct FieldInfo;

		struct StructuredDataRow {
			uint16_t fieldCount;
			FieldInfo *fields;
			TemplateId templateId;
		};

		/**
		 * Information describing a single field in the Template
		 */
		struct FieldInfo {
			InformationElement::IeInfo type;
			int32_t offset; 	/**< offset in bytes from a data start pointer. For internal purposes 0xFFFFFFFF is defined as yet unknown */
			int32_t privDataOffset; /**< offset in bytes from data start pointer for internal private data which is not exported via IPFIX */
			bool isVariableLength; 	/**< true if this field's length might change from record to record, false otherwise */
			// The following fields add support for structured data (see RFC 6313)
			uint8_t semantic; /** The semantic of the structured data element. See the RFC for more information */
			uint16_t rowCount; /** Due to the IPFix maximum message size there cannot be more than 2^16 - 1 rows. The value of this field must be ignored if \a rows is NULL. */
			StructuredDataRow *rows; /** Array of structured data rows */
		};

		TemplateInfo();
		TemplateInfo(const TemplateInfo& t);
		~TemplateInfo();

		void setUniqueId();
		inline uint16_t getUniqueId() {
			return uniqueId;
		}

		FieldInfo* getFieldInfo(const InformationElement::IeInfo& type);
		FieldInfo* getFieldInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid);
		int getFieldIndex(const InformationElement::IeInfo& type);
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

		/**
		  * Allocates and initializes a FieldInfo array with the capacity \a numberOfRecrods.
		  *
		  * \returns A pointer to the beginning of the allocated memory region or NULL if the
		  *          memory could not be allocated.
		  */
		static FieldInfo *allocateFieldInfoArray(size_t numberOfRecords);
		/**
		  * Allocates and initializes a StructuredDataRow array with the capacity \a numberOfRecrods.
		  *
		  * \returns A pointer to the beginning of the allocated memory region or NULL if the
		  *          memory could not be allocated.
		  */
		static StructuredDataRow *allocateStructuredDataRowArray(size_t numberOfRecords);
	private:
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
		 * - requires long lifetime as it is used in the destructor of TemplateInfo (singleton without destruction)
		 *   see: http://groups.google.com/group/comp.lang.c++/browse_thread/thread/8c6c8d00ec467068/fd8778f91ef7397e?lnk=raot
		 */
		std::vector<uint16_t>& uniqueIdUseCount() {
			static std::vector<uint16_t>* theOnlyUniqueIdUseCount = new std::vector<uint16_t>;
			return *theOnlyUniqueIdUseCount;
		}

		/* mutex:
		 * - controls access to uniqueIdUseCoune
		 * - requires long lifetime as it is used in the destructor of TemplateInfo (singleton without destruction)
		 */
		Mutex& mutex() {
			static Mutex* theOnlyMutex = new Mutex;
			return *theOnlyMutex;
		}

		/**
		  * Frees the memory allocated by the given \a fields array.
		  */
		void freeFieldInfo(uint16_t fieldCount, TemplateInfo::FieldInfo *fields) const;

		/**
		  * Frees the memory allocated by the given \a rows array.
		  */
		void freeStructuredDataRows(uint16_t rowCount, TemplateInfo::StructuredDataRow *rows) const;

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

