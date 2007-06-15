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
#include "../sampler/Lock.h"
#include "../sampler/Packet.h"

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
			int32_t offset; /**< offset in bytes from a data start pointer. For internal purposes 0xFFFFFFFF is defined as yet unknown */
		};


		/**
		 * Template description passed to the callback function when a new Template arrives.
		 */
		struct TemplateInfo {
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
			 * gets a types length
			 **/
			int getFieldLength(IpfixRecord::FieldInfo::Type type) {

			int type_length;

			switch (type.id) {
			case IPFIX_TYPEID_packetDeltaCount:
			type_length = 1;
			return type_length;
			break;

			case IPFIX_TYPEID_flowStartSeconds:
			type_length = 4;
			return type_length;
			break;

			case IPFIX_TYPEID_flowStartMilliSeconds:
			type_length = 8;
			return type_length;
			break;

			case IPFIX_TYPEID_flowEndSeconds:
			type_length = 4;
			return type_length;
			break;

			case IPFIX_TYPEID_flowEndMilliSeconds:
			type_length = 8;
			return type_length;
			break;

			case IPFIX_TYPEID_octetDeltaCount:
			type_length = 2;
			return type_length;
			break;

			case IPFIX_TYPEID_protocolIdentifier:
			type_length = 1;
			return type_length;
			break;

			case IPFIX_TYPEID_sourceIPv4Address:
			type_length = 4;
			return type_length;
			break;

			case IPFIX_TYPEID_destinationIPv4Address:
			type_length = 4;
			return type_length;
			break;

			case IPFIX_TYPEID_icmpTypeCode:
			type_length = 4;
			return type_length;
			break;

			case IPFIX_TYPEID_sourceTransportPort:
			type_length = 2;
			return type_length;
			break;
	
			case IPFIX_TYPEID_destinationTransportPort:
			type_length = 2;
			return type_length;
			break;

			case IPFIX_TYPEID_tcpControlBits:
			type_length = 1;
			return type_length;
			break;

			default:
			return 999;
			break;
			}
	
			return 999;
			}



			IpfixRecord::Data* getFieldPointer(IpfixRecord::FieldInfo::Type type, IpfixRecord::Data* ip_data, IpfixRecord::Data* th_data, int classi) {
				
				//IpfixRecord::Data* idata=(IpfixRecord::Data *)pdata->shit;


				switch (type.id) {
				case IPFIX_TYPEID_packetDeltaCount:
				return ip_data + 10;
				break;

				case IPFIX_TYPEID_flowStartSeconds:
				return ip_data + 4;	
				break;

				case IPFIX_TYPEID_flowStartMilliSeconds:
				return ip_data + 0;	
				break;

				case IPFIX_TYPEID_flowEndSeconds:
				return ip_data + 4;
				break;

				case IPFIX_TYPEID_flowEndMilliSeconds:
				return ip_data + 0;
				break;

				case IPFIX_TYPEID_octetDeltaCount:
				return ip_data + 2;
				break;

				case IPFIX_TYPEID_protocolIdentifier:
				return ip_data + 9;
				break;

				case IPFIX_TYPEID_sourceIPv4Address:
				return ip_data + 12;
				break;

				case IPFIX_TYPEID_destinationIPv4Address:
				return ip_data + 16;
				break;

				case IPFIX_TYPEID_icmpTypeCode:
				if(classi == 0) {
//					IpfixRecord::Data* tdata=(IpfixRecord::Data *)pdata->transport_header;
					return th_data + 0;
				} else {
					return NULL;
				}
				break;

				case IPFIX_TYPEID_sourceTransportPort:
				if((classi == 1) || (classi == 2)) {
				//	IpfixRecord::Data* tdata=(IpfixRecord::Data *)pdata->transport_header;
					return th_data + 0;
				} else {
					return NULL;
				}
				break;

				case IPFIX_TYPEID_destinationTransportPort:
				if((classi == 1) || (classi == 2)) {
				//	IpfixRecord::Data* tdata=(IpfixRecord::Data *)pdata->transport_header;
					return th_data + 2;
				} else {
					return NULL;
				}
				break;

				case IPFIX_TYPEID_tcpControlBits:
				if(classi == 1) {
				//	IpfixRecord::Data* tdata=(IpfixRecord::Data *)pdata->transport_header;
					return th_data + 13;
				} else {
					return NULL;
				}
				break;	

				default:
				return NULL;
				break;
				}

				return NULL;

			}

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
		struct DataTemplateInfo {
			DataTemplateInfo() : freePointers(true) {
			}

			~DataTemplateInfo() {
				if (freePointers) {
				    free(fieldInfo);
				    free(dataInfo);
				    free(data);
				}
			}

			IpfixRecord::FieldInfo* getFieldInfo(IpfixRecord::FieldInfo::Type* type) {
				return getFieldInfo(type->id, type->eid);
			}



			/**
			 * Gets a DataTemplate's FieldInfo by field id. Length is ignored.
			 * @param fieldTypeId Field id to look for
			 * @param fieldTypeEid Field eid to look for
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

			uint16_t templateId; /**< the template id assigned to this template or 0 if we don't know or don't care */
			uint16_t preceding; /**< the preceding rule field as defined in the draft */
			uint16_t fieldCount; /**< number of regular fields */
			IpfixRecord::FieldInfo* fieldInfo; /**< array of FieldInfos describing each of these fields */
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

