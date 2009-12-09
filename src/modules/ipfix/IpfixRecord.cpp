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

#include "common/msg.h"
#include "IpfixRecord.hpp"


namespace InformationElement {

	/**
	 * Gets standard length for a standard information element id
	 * TODO: tidy up, this function is equivalent to ipfix_names.c:ipfix_id_lookup(int)
	 * @param id Standard Information Element ID (registered by IANA)
	 * @returns Standard length of the Information Element (65535 = variable length by default)
	 */
	IeLength getFieldLength(IeId id) {
		switch (id) {
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
			case IPFIX_ETYPEID_maxPacketGap:
			case IPFIX_ETYPEID_revFlowStartSeconds:
			case IPFIX_ETYPEID_revFlowEndSeconds:
			case IPFIX_ETYPEID_revMaxPacketGap:
			case IPFIX_ETYPEID_frontPayloadPktCount:
				return 4;

			case IPFIX_TYPEID_flowStartMilliSeconds:
			case IPFIX_TYPEID_flowEndMilliSeconds:
			case IPFIX_TYPEID_octetDeltaCount:
			case IPFIX_TYPEID_packetDeltaCount:
			case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			case IPFIX_ETYPEID_revFlowEndMilliSeconds:
			case IPFIX_ETYPEID_revFlowStartNanoSeconds:
			case IPFIX_ETYPEID_revFlowEndNanoSeconds:
			case IPFIX_ETYPEID_revOctetDeltaCount:
			case IPFIX_ETYPEID_revPacketDeltaCount:
				return 8;
		}
		THROWEXCEPTION("IpfixRecord: unknown typeid, failed to determine field length");
		return 0;
	}

	/**
	 * Checks whether the given Field carries reverse flow information in a bi-flow record
	 * TODO: According to the standard, reversef low IEs should use enterprise number 29305
	 * @param type Data Record field
	 * @returns true if this IE carries reverse flow information
	 */
	bool isBiflowField(const IeInfo& type)
	{
		// TODO: Bi-flow standard: if(field.type ==29305) return true;
		switch (type.id) {
			case IPFIX_ETYPEID_revTcpControlBits:
			case IPFIX_ETYPEID_revFlowStartSeconds:
			case IPFIX_ETYPEID_revFlowEndSeconds:
			case IPFIX_ETYPEID_revFlowStartMilliSeconds:
			case IPFIX_ETYPEID_revFlowEndMilliSeconds:
			case IPFIX_ETYPEID_revFlowStartNanoSeconds:
			case IPFIX_ETYPEID_revFlowEndNanoSeconds:
			case IPFIX_ETYPEID_revOctetDeltaCount:
			case IPFIX_ETYPEID_revPacketDeltaCount:
			case IPFIX_ETYPEID_revMaxPacketGap:
				return true;
		}

		return false;
	}

	/**
	 * Returns IE ID of opposite direction.
	 * @param type Data Record field
	 * @returns IeId of IE in opposite direction, or zero if does not exist
	 */
	IeId oppositeDirectionIeId(const IeInfo& type)
	{
		if (type.enterprise == 0)
			switch(type.id) {
				case IPFIX_TYPEID_sourceIPv4Address:
					return IPFIX_TYPEID_destinationIPv4Address;
				case IPFIX_TYPEID_destinationIPv4Address:
					return IPFIX_TYPEID_sourceIPv4Address;
				case IPFIX_TYPEID_sourceIPv6Address:
					return IPFIX_TYPEID_destinationIPv6Address;
				case IPFIX_TYPEID_destinationIPv6Address:
					return IPFIX_TYPEID_sourceIPv6Address;
				case IPFIX_TYPEID_sourceIPv4Mask:
					return IPFIX_TYPEID_destinationIPv4Mask;
				case IPFIX_TYPEID_destinationIPv4Mask:
					return IPFIX_TYPEID_sourceIPv4Mask;
				case IPFIX_TYPEID_sourceIPv6Mask:
					return IPFIX_TYPEID_destinationIPv6Mask;
				case IPFIX_TYPEID_destinationIPv6Mask:
					return IPFIX_TYPEID_sourceIPv6Mask;
				case IPFIX_TYPEID_sourceIPv4Prefix:
					return IPFIX_TYPEID_destinationIPv4Prefix;
				case IPFIX_TYPEID_destinationIPv4Prefix:
					return IPFIX_TYPEID_sourceIPv4Prefix;
				//case IPFIX_TYPEID_sourceIPv6Prefix:
				//	return IPFIX_TYPEID_destinationIPv6Prefix;
				//case IPFIX_TYPEID_destinationIPv6Prefix:
				//	return IPFIX_TYPEID_sourceIPv6Prefix;
				case IPFIX_TYPEID_sourceTransportPort:
					return IPFIX_TYPEID_destinationTransportPort;
				case IPFIX_TYPEID_destinationTransportPort:
					return IPFIX_TYPEID_sourceTransportPort;
				case IPFIX_TYPEID_udpSourcePort:
					return IPFIX_TYPEID_udpDestinationPort;
				case IPFIX_TYPEID_udpDestinationPort:
					return IPFIX_TYPEID_udpSourcePort;
				case IPFIX_TYPEID_tcpSourcePort:
					return IPFIX_TYPEID_tcpDestinationPort;
				case IPFIX_TYPEID_tcpDestinationPort:
					return IPFIX_TYPEID_tcpSourcePort;
		}
		return 0;
	}

	/**
	 * Returns valid protocols for given field
	 * @param field Data Record field
	 * @param p pointer to raw packet
	 * @returns offset (in bytes) at which the data for the given field is located in the raw packet
	 */
	const Packet::IPProtocolType getValidProtocols(const IeInfo& type)
	{
		if(type.enterprise == 0) {
			switch (type.id) {
				case IPFIX_TYPEID_packetDeltaCount:
				case IPFIX_TYPEID_flowStartSeconds:
				case IPFIX_TYPEID_flowEndSeconds:
				case IPFIX_TYPEID_flowStartMilliSeconds:
				case IPFIX_TYPEID_flowEndMilliSeconds:
				case IPFIX_TYPEID_flowStartNanoSeconds:
				case IPFIX_TYPEID_flowEndNanoSeconds:
				case IPFIX_TYPEID_octetDeltaCount:
				case IPFIX_TYPEID_protocolIdentifier:
				case IPFIX_TYPEID_sourceIPv4Address:
				case IPFIX_TYPEID_destinationIPv4Address:
				case IPFIX_ETYPEID_revPacketDeltaCount:
				case IPFIX_ETYPEID_revFlowStartSeconds:
				case IPFIX_ETYPEID_revFlowEndSeconds:
				case IPFIX_ETYPEID_revFlowStartMilliSeconds:
				case IPFIX_ETYPEID_revFlowEndMilliSeconds:
				case IPFIX_ETYPEID_revFlowStartNanoSeconds:
				case IPFIX_ETYPEID_revFlowEndNanoSeconds:
				case IPFIX_ETYPEID_revOctetDeltaCount:
				case IPFIX_ETYPEID_maxPacketGap:
				case IPFIX_ETYPEID_revMaxPacketGap:
					return Packet::IPProtocolType(Packet::TCP|Packet::UDP|Packet::ICMP);

				case IPFIX_TYPEID_icmpTypeCode:
					return Packet::ICMP;

				case IPFIX_TYPEID_sourceTransportPort:
				case IPFIX_TYPEID_destinationTransportPort:
				case IPFIX_ETYPEID_frontPayload:
				case IPFIX_ETYPEID_frontPayloadLen:
				case IPFIX_ETYPEID_revFrontPayload:
				case IPFIX_ETYPEID_revFrontPayloadLen:
				case IPFIX_ETYPEID_frontPayloadPktCount:
					return Packet::IPProtocolType(Packet::UDP|Packet::TCP);

				case IPFIX_TYPEID_tcpControlBits:
				case IPFIX_ETYPEID_revTcpControlBits:
					return Packet::TCP;
			}
		}
		THROWEXCEPTION("received unknown field type id (%d)", type.id);
		return Packet::NONE;
	}

}


/* Methods  of TemplateInfo class */
std::vector<uint16_t> TemplateInfo::uniqueIdUseCount(64,0);   // initialize with 64 Ids

TemplateInfo::TemplateInfo() : templateId(0), setId(UnknownSetId), fieldCount(0), fieldInfo(NULL), 
        destroyed(false), freePointers(true),
	scopeCount(0), scopeInfo(NULL), dataCount(0), dataInfo(NULL), preceding(0), dataLength(0), data(NULL), 
	anonymized(false), uniqueId(0)
{
        setUniqueId();
}

TemplateInfo::TemplateInfo(const TemplateInfo& t)
{
	templateId = t.templateId;
	setId = t.setId;

	// copy fields
	fieldCount = t.fieldCount; /**< number of regular fields */
	fieldInfo = (FieldInfo*)malloc(fieldCount*sizeof(FieldInfo));
	memcpy(fieldInfo, t.fieldInfo, fieldCount*sizeof(FieldInfo));

	destroyed = t.destroyed;
	freePointers = t.freePointers;

	// copy Options Template scope fields
	scopeCount = t.scopeCount;
	scopeInfo = (FieldInfo*)malloc(scopeCount*sizeof(FieldInfo));
	memcpy(scopeInfo, t.scopeInfo, scopeCount*sizeof(FieldInfo));

	// copy Data Template data fields and further attributes
	dataCount = t.dataCount;
	dataInfo = (FieldInfo*)malloc(dataCount*sizeof(FieldInfo));
	memcpy(dataInfo, t.dataInfo, dataCount*sizeof(FieldInfo));
	preceding = t.preceding;
	dataLength = t.dataLength;
	data = (IpfixRecord::Data*)malloc(dataLength*sizeof(IpfixRecord::Data));
	memcpy(data, t.data, dataLength*sizeof(IpfixRecord::Data));
	anonymized = t.anonymized;

	// copy uniqueId (a new uniqueId can be assigned with setUniqueId() if needed)
	uniqueId = t.uniqueId;
	// increase reference count in uniqueIdUseCount
	mutex.lock();
	if((uniqueId == 0) || (uniqueId>uniqueIdUseCount.size()) || (uniqueIdUseCount[uniqueId-1]==0))
		THROWEXCEPTION("TemplateInfo copy constructor: uniqueIdUseCount is corrupt, this should never happen (Template ID=%u)!", templateId);
	if(uniqueIdUseCount[uniqueId-1]==65535)
		THROWEXCEPTION("TemplateInfo: uniqueIdUseCount uint16_t overflow!");
	uniqueIdUseCount[uniqueId-1]++;
	mutex.unlock();
}

TemplateInfo::~TemplateInfo() {
	if (freePointers)
	{
		free(fieldInfo);
		free(scopeInfo);
		free(dataInfo);
		free(data);
		freePointers = false;
	}
	// decrease reference count in uniqueIdUseCount
	mutex.lock();
	if((uniqueId == 0) || (uniqueId>uniqueIdUseCount.size()) || (uniqueIdUseCount[uniqueId-1]==0))
		THROWEXCEPTION("TemplateInfo destructor: uniqueIdUseCount is corrupt, this should never happen!");
	uniqueIdUseCount[uniqueId-1]--;
	mutex.unlock();
}

void TemplateInfo::setUniqueId() {
	mutex.lock();
	uint16_t oldId = uniqueId;
	uint16_t oldSize = uniqueIdUseCount.size();

	uniqueId = 0;
	for(int i=0; i < oldSize; i++) {
		if(uniqueIdUseCount[i] == 0) {
			uniqueIdUseCount[i] = 1;
			uniqueId = i+1; // the id is i+1
			break;
		}
	}
	// if new element was zero, we need to add a new element to uniqueIdUseCount
	if(uniqueId == 0) {
		if(oldSize == 65535)
			THROWEXCEPTION("TemplateInfo: more than 65353 uniqueIds needed");
		msg(MSG_VDEBUG, "TemplateInfo: need to increase number of uniqueIds");
		uniqueIdUseCount.push_back(1);
		uniqueId = oldSize + 1;
	}

	// release old uniqueId if set
	if(oldId != 0) {
		if((oldId >= oldSize) || (uniqueIdUseCount[oldId-1]==0))
			THROWEXCEPTION("TemplateInfo setUniqueId: uniqueIdUseCount is corrupt, this should never happen!");
		uniqueIdUseCount[oldId-1]--;
	}
	mutex.unlock();
}

/**
 * Gets a Template's FieldInfo by Information Element id. Length is ignored.
 * @param type Information Element to look for. Length is ignored.
 * @return NULL if not found
 */
TemplateInfo::FieldInfo* TemplateInfo::getFieldInfo(const InformationElement::IeInfo& type) {
	return getFieldInfo(type.id, type.enterprise);
}

//			IpfixRecord::Data* TemplateInfo::getFieldPointer(InformationElement::IeInfo* type, IpfixRecord::Data* pdata) {
//				return getFieldPointer(type->id, &pdata);
//			}
//

/**
 * Gets a Template's FieldInfo by IE Id and enterprise number.
 * @param fieldTypeId Information element Id to look for
 * @param fieldTypeEid Enterprise number to look for
 * @return NULL if not found
 */
TemplateInfo::FieldInfo* TemplateInfo::getFieldInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid) {
	int i;

	for (i = 0; i < fieldCount; i++) {
		if ((fieldInfo[i].type.id == fieldTypeId) && (fieldInfo[i].type.enterprise == fieldTypeEid)) {
			return &fieldInfo[i];
		}
	}

	return NULL;
}

/**
 * Gets position of a field in the Template.
 * @param fieldTypeId Information element Id to look for
 * @param fieldTypeEid Enterprise number to look for
 * @return NULL if not found
 */
int TemplateInfo::getFieldIndex(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid) {
	int i;

	for (i = 0; i < fieldCount; i++) {
		if ((fieldInfo[i].type.id == fieldTypeId) && (fieldInfo[i].type.enterprise == fieldTypeEid)) {
			return i;
		}
	}

	return -1;
}

/**
 * Gets data of Data Templates for a given Information Element.
 * @param type Information Element to get data for
 * @return NULL if not found
 */
TemplateInfo::FieldInfo* TemplateInfo::getDataInfo(const InformationElement::IeInfo& type) {
	return getDataInfo(type.id, type.enterprise);
}

/**
 * Gets data of Data Templates for a given Information Element Id and enterprise number.
 * @param fieldTypeId Information Element id to look for
 * @param fieldTypeEid enterprise number to look for
 * @return NULL if not found
 */
TemplateInfo::FieldInfo* TemplateInfo::getDataInfo(InformationElement::IeId fieldTypeId, InformationElement::IeEnterpriseNumber fieldTypeEid) {
	int i;

	for (i = 0; i < dataCount; i++) {
		if ((dataInfo[i].type.id == fieldTypeId) && (dataInfo[i].type.enterprise == fieldTypeEid)) {
			return &dataInfo[i];
		}
	}

	return NULL;
}

