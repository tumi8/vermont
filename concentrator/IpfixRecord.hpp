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

#include "ipfix.hpp"
#include "sampler/Packet.h"

#define MAX_ADDRESS_LEN 16

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
			int32_t offset; /**< offset in bytes from a data start pointer. For internal purposes 0xFFFFFFFF is defined as yet unknown */
		};


		/**
		 * Template description passed to the callback function when a new Template arrives.
		 */
		struct TemplateInfo {
			TemplateInfo() : destroyed(false) {	}
			
			~TemplateInfo() {
				free(fieldInfo);
			}

			/**
			 * Gets a Template's FieldInfo by field id. Length is ignored.
			 * @param type Field id and eid to look for. Length is ignored.
			 * @return NULL if not found
			 */
			IpfixRecord::FieldInfo* getFieldInfo(IpfixRecord::FieldInfo::Type* type) {
				return getFieldInfo(type->id, type->eid);
			}

//			IpfixRecord::Data* getFieldPointer(IpfixRecord::FieldInfo::Type* type, IpfixRecord::Data* pdata) {
//				return getFieldPointer(type->id, &pdata);
//			}
			/**
			 * Gets a Template's FieldInfo by field id. Length is ignored.
			 * @param fieldTypeId FieldInfo::Type id to look for
			 * @param fieldTypeEid FieldInfo::Type eid to look for
			 * @return NULL if not found
			 */
			IpfixRecord::FieldInfo* getFieldInfo(IpfixRecord::FieldInfo::Type::Id fieldTypeId, IpfixRecord::FieldInfo::Type::EnterpriseNo fieldTypeEid) {
				int i;

				for (i = 0; i < fieldCount; i++) {
					if ((fieldInfo[i].type.id == fieldTypeId) && (fieldInfo[i].type.eid == fieldTypeEid)) {
						return &fieldInfo[i];
					}
				}

				return NULL;
			}

			/**
			 * gets a type's length
			 **/
			static int getFieldLength(IpfixRecord::FieldInfo::Type type) {

				switch (type.id) {					
					case IPFIX_TYPEID_protocolIdentifier:
					case IPFIX_TYPEID_tcpControlBits:
					case IPFIX_ETYPEID_revTcpControlBits:
						return 1;
						
					case IPFIX_TYPEID_icmpTypeCode:
					case IPFIX_TYPEID_sourceTransportPort:
					case IPFIX_TYPEID_destinationTransportPort:
						return 2;

					case IPFIX_TYPEID_flowStartSeconds:
					case IPFIX_TYPEID_flowEndSeconds:
					case IPFIX_TYPEID_sourceIPv4Address:
					case IPFIX_TYPEID_destinationIPv4Address:
					case IPFIX_ETYPEID_revFlowStartSeconds:
					case IPFIX_ETYPEID_revFlowEndSeconds:
						return 4;

					case IPFIX_TYPEID_flowStartMilliSeconds:					
					case IPFIX_TYPEID_flowEndMilliSeconds:					
					case IPFIX_TYPEID_octetDeltaCount:
					case IPFIX_TYPEID_packetDeltaCount:
					case IPFIX_ETYPEID_revFlowStartMilliSeconds:
					case IPFIX_ETYPEID_revFlowEndMilliSeconds:
					case IPFIX_ETYPEID_revOctetDeltaCount:
					case IPFIX_ETYPEID_revPacketDeltaCount:
						return 8;

					default:
						THROWEXCEPTION("unknown typeid");
						break;
				}

				THROWEXCEPTION("unknown typeid");
				return 0;
			}
			
			/**
			 * @returns an index for the given field type which is relative to the start of the Packet's netheader
			 */
			static const uint16_t getRawPacketFieldIndex(uint16_t typeId, const Packet* p) 
			{
				switch (typeId) {
					case IPFIX_TYPEID_packetDeltaCount:
						return 10;
						break;

					case IPFIX_TYPEID_flowStartSeconds:
					case IPFIX_TYPEID_flowEndSeconds:
						return reinterpret_cast<const unsigned char*>(&p->time_sec_nbo) - p->netHeader;
						break;

					case IPFIX_TYPEID_flowStartMilliSeconds:
					case IPFIX_TYPEID_flowEndMilliSeconds:
						return reinterpret_cast<const unsigned char*>(&p->time_msec_nbo) - p->netHeader;
						break;

					case IPFIX_TYPEID_octetDeltaCount:
						return 2;
						break;

					case IPFIX_TYPEID_protocolIdentifier:
						return 9;
						break;

					case IPFIX_TYPEID_sourceIPv4Address:
						return 12;
						break;

					case IPFIX_TYPEID_destinationIPv4Address:
						return 16;
						break;

					case IPFIX_TYPEID_icmpTypeCode:
						if(p->ipProtocolType == Packet::ICMP) {
							return p->transportHeader + 0 - p->netHeader;
						} else {
							DPRINTFL(MSG_VDEBUG, "given typeid is %d, protocol type is %d, but expected was %d or %d", typeId, p->ipProtocolType, Packet::UDP, Packet::TCP);

						}
						break;

					case IPFIX_TYPEID_sourceTransportPort:
						if((p->ipProtocolType == Packet::TCP) || (p->ipProtocolType == Packet::UDP)) {
							return p->transportHeader + 0 - p->netHeader;
						} else {
							DPRINTFL(MSG_VDEBUG, "given typeid is %d, protocol type is %d, but expected was %d or %d", typeId, p->ipProtocolType, Packet::UDP, Packet::TCP);
						}
						break;

					case IPFIX_TYPEID_destinationTransportPort:						if((p->ipProtocolType == Packet::TCP) || (p->ipProtocolType == Packet::UDP)) {
							return p->transportHeader + 2 - p->netHeader;
						} else {
							THROWEXCEPTION("given typeid is %d, protocol type is %d, but expected was %d or %d", typeId, p->ipProtocolType, Packet::UDP, Packet::TCP);
						}
						break;

					case IPFIX_TYPEID_tcpControlBits:
						if(p->ipProtocolType == Packet::TCP) {
							return p->transportHeader + 13 - p->netHeader;
						} else {
							THROWEXCEPTION("given typeid is %d, protocol type is %d, but expected was %d", typeId, p->ipProtocolType, Packet::TCP);
						}
						break;	
				}

				THROWEXCEPTION("unknown typeid (%d)", typeId);
				return 0;
			}


			static const Packet::IPProtocolType getValidProtocols(uint16_t typeId)
			{
				switch (typeId) {
					case IPFIX_TYPEID_packetDeltaCount:
					case IPFIX_TYPEID_flowStartSeconds:
					case IPFIX_TYPEID_flowEndSeconds:
					case IPFIX_TYPEID_flowStartMilliSeconds:
					case IPFIX_TYPEID_flowEndMilliSeconds:
					case IPFIX_TYPEID_octetDeltaCount:
					case IPFIX_TYPEID_protocolIdentifier:
					case IPFIX_TYPEID_sourceIPv4Address:
					case IPFIX_TYPEID_destinationIPv4Address:
					case IPFIX_ETYPEID_revFlowEndMilliSeconds:
					case IPFIX_ETYPEID_revFlowEndSeconds:
					case IPFIX_ETYPEID_revFlowStartMilliSeconds:
					case IPFIX_ETYPEID_revFlowStartSeconds:
					case IPFIX_ETYPEID_revOctetDeltaCount:
					case IPFIX_ETYPEID_revPacketDeltaCount:
					case IPFIX_ETYPEID_revTcpControlBits:
						
						return Packet::IPProtocolType(Packet::TCP|Packet::UDP|Packet::ICMP);

					case IPFIX_TYPEID_icmpTypeCode:
						return Packet::ICMP;

					case IPFIX_TYPEID_sourceTransportPort:
					case IPFIX_TYPEID_destinationTransportPort:
						return Packet::IPProtocolType(Packet::UDP|Packet::TCP);

					case IPFIX_TYPEID_tcpControlBits:
						return Packet::TCP;
				}
				THROWEXCEPTION("received unknown field type id (%d)", typeId);
				return Packet::NONE;
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
		};

		/**
		 * OptionsTemplate description passed to the callback function when a new OptionsTemplate arrives.
		 * Note that - other than in [PROTO] - fieldCount specifies only the number of regular fields
		 */
		struct OptionsTemplateInfo {
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
			DataTemplateInfo() : freePointers(true) 
			{
			}

			~DataTemplateInfo() {
				if (freePointers) {
				    free(fieldInfo);
				    free(dataInfo);
				    free(data);
				}
			}


			IpfixRecord::FieldInfo* getDataInfo(IpfixRecord::FieldInfo::Type* type) {
				return getDataInfo(type->id, type->eid);
			}

			/**
			 * Gets a DataTemplate's Data-FieldInfo by field id.
			 * @param fieldTypeId Field id to look for
			 * @param fieldTypeEid Field eid to look for
			 * @return NULL if not found
			 */
			IpfixRecord::FieldInfo* getDataInfo(IpfixRecord::FieldInfo::Type::Id fieldTypeId, IpfixRecord::FieldInfo::Type::EnterpriseNo fieldTypeEid) {
				int i;

				for (i = 0; i < dataCount; i++) {
					if ((dataInfo[i].type.id == fieldTypeId) && (dataInfo[i].type.eid == fieldTypeEid)) {
						return &dataInfo[i];
					}
				}

				return NULL;		
			}

			uint16_t preceding; /**< the preceding rule field as defined in the draft */
			uint16_t dataCount; /**< number of fixed-value fields */
			IpfixRecord::FieldInfo* dataInfo; /**< array of FieldInfos describing each of these fields */
			IpfixRecord::Data* data; /**< data start pointer for fixed-value fields */
			void* userData; /**< pointer to a field that can be used by higher-level modules */
			bool freePointers;  /** small helper variable to indicate if pointers should be freed on destruction */
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

