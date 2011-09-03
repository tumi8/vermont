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

#include <string>

using namespace std;


namespace InformationElement {

	/**
	 * Checks whether the given Field carries reverse flow information in a bi-flow record
	 * @returns true if this IE carries reverse flow information
	 */
	bool IeInfo::isReverseField() const
	{
		return enterprise & IPFIX_PEN_reverse;
		// return (enterprise == IPFIX_PEN_reverse) || ((enterprise == IPFIX_PEN_vermont) && (id & IPFIX_ETYPE_reverse_bit))
	}

	/**
	 * @returns true if given ie exists in the reverse direction
	 */
	bool IeInfo::existsReverseDirection()
	{
		// check if type was declared in ipfix.h
		if (ipfix_id_lookup(id, enterprise | IPFIX_PEN_reverse)==0) return false;

		return true;

		//if ((enterprise == 0) && (ipfix_id_lookup(id, IPFIX_PEN_reverse) != 0)) return true;
		//if ((enterprise == IPFIX_PEN_reverse) && (ipfix_id_lookup(id, 0) != 0)) return true;
		//if ((enterprise == IPFIX_PEN_vermont) && (ipfix_id_lookup(id^IPFIX_ETYPE_reverse_bit, IPFIX_PEN_vermont) != 0)) return true;
		//return false;
	}

	/**
	 * Returns IE ID of reverse direction (ATTENTION: direction change from reverse -> normal is NOT possible).
	 * ATTENTION: this method stops Vermont if no field was found in the opposite direction
	 * @returns IeId of IE in opposite direction
	 */
	IeInfo IeInfo::getReverseDirection()
	{
		if (enterprise & IPFIX_PEN_reverse) THROWEXCEPTION("requested reverse direction of enterprise type id %s", toString().c_str());

		// check if type was declared in ipfix.h
		if (ipfix_id_lookup(id, enterprise | IPFIX_PEN_reverse)==0)
			THROWEXCEPTION("did not find reverse element for %s", toString().c_str());

		return IeInfo(id, enterprise | IPFIX_PEN_reverse, length);

		//ipfix_identifier* id;
		//if ((enterprise == 0) && ((id = ipfix_id_lookup(id, IPFIX_PEN_reverse)) != 0)) return IeInfo(id);
		//if ((enterprise == IPFIX_PEN_reverse) && ((id = ipfix_id_lookup(id, 0)) != 0)) return IeInfo(id);
		//if ((enterprise == IPFIX_PEN_vermont) && ((id = ipfix_id_lookup(id^IPFIX_ETYPE_reverse_bit, IPFIX_PEN_vermont)) != 0)) return IeInfo(id);
		//THROWEXCEPTION("did not find reverse element for %s", toString().c_str());
	}

	/**
	 * @returns valid protocols for given field, binary ORed if multiple protocols are possible
	 */
	const Packet::IPProtocolType IeInfo::getValidProtocols()
	{
		if (enterprise==0 || enterprise==IPFIX_PEN_reverse) {
			switch (id) {
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
				case IPFIX_ETYPEID_maxPacketGap:
					return Packet::IPProtocolType(Packet::TCP|Packet::UDP|Packet::ICMP);

				case IPFIX_TYPEID_icmpTypeCodeIPv4:
					return Packet::ICMP;

				case IPFIX_TYPEID_sourceTransportPort:
				case IPFIX_TYPEID_destinationTransportPort:
					return Packet::IPProtocolType(Packet::UDP|Packet::TCP);

				case IPFIX_TYPEID_tcpControlBits:
					return Packet::TCP;
			}
		} else if (enterprise==IPFIX_PEN_vermont || enterprise==(IPFIX_PEN_vermont|IPFIX_PEN_reverse)) {
			// Vermont-specific type-ids
			switch (id) {
				case IPFIX_ETYPEID_maxPacketGap:
					return Packet::IPProtocolType(Packet::TCP|Packet::UDP|Packet::ICMP);

				case IPFIX_ETYPEID_frontPayload:
				case IPFIX_ETYPEID_frontPayloadLen:
				case IPFIX_ETYPEID_frontPayloadPktCount:
				case IPFIX_ETYPEID_dpaForcedExport:
				case IPFIX_ETYPEID_dpaFlowCount:
				case IPFIX_ETYPEID_dpaReverseStart:
				case IPFIX_ETYPEID_transportOctetDeltaCount:
					return Packet::IPProtocolType(Packet::UDP|Packet::TCP);
			}
			if (enterprise==IPFIX_PEN_vermont) {
				switch (id) {
					case IPFIX_ETYPEID_anonymisationType:
						return Packet::IPProtocolType(Packet::UDP|Packet::TCP);
				}
			}
		}
		THROWEXCEPTION("received unknown field type %s", toString().c_str());
		return Packet::NONE;
	}

	string IeInfo::toString() const
	{
		char buffer[100];
		const ipfix_identifier* ipfixid = ipfix_id_lookup(id, enterprise);
		if (enterprise == 0)
			snprintf(buffer, ARRAY_SIZE(buffer), "%s (id=%hu, length=%hu)",
				ipfixid ? ipfixid->name : "unknown", id, length);
		else if (enterprise == IPFIX_PEN_reverse)
			snprintf(buffer, ARRAY_SIZE(buffer), "%s (id=%hu, pen=%u [reverse], length=%hu)",
				ipfixid ? ipfixid->name : "unknown", id, enterprise, length);
		else if (enterprise == IPFIX_PEN_vermont)
			snprintf(buffer, ARRAY_SIZE(buffer), "%s (id=%hu, pen=%u [vermont], length=%hu)",
				ipfixid ? ipfixid->name : "unknown", id, enterprise, length);
		else
			snprintf(buffer, ARRAY_SIZE(buffer), "%s (id=%hu, pen=%u, length=%hu)",
				ipfixid ? ipfixid->name : "unknown", id, enterprise, length);
		return buffer;
	}

	/**
	  * Returns whether this Information Element type is a structured data type
	  * (e.g. basicList, subTemplateList, multiSubTemplateList)
	  */
	bool IeInfo::isStructuredData() const {
		if (enterprise != 0)
			return false;

		switch (id) {
		case IPFIX_TYPEID_basicList:
		case IPFIX_TYPEID_subTemplateList:
		case IPFIX_TYPEID_subTemplateMultiList:
			return true;
		default:
			return false;
		}
	}

}


/* Methods  of TemplateInfo class */

TemplateInfo::TemplateInfo() : templateId(0), setId(UnknownSetId), fieldCount(0), fieldInfo(NULL),
        freePointers(true),
	scopeCount(0), scopeInfo(NULL), dataCount(0), dataInfo(NULL), preceding(0), dataLength(0), data(NULL),
	uniqueId(0)
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

	// copy uniqueId (a new uniqueId can be assigned with setUniqueId() if needed)
	uniqueId = t.uniqueId;
	// increase reference count in uniqueIdUseCount()
	mutex().lock();
	if((uniqueId == 0) || (uniqueId>uniqueIdUseCount().size()) || (uniqueIdUseCount()[uniqueId-1]==0))
		THROWEXCEPTION("TemplateInfo copy constructor: uniqueIdUseCount is corrupt, this should never happen (Template ID=%u)!", templateId);
	if(uniqueIdUseCount()[uniqueId-1]==65535)
		THROWEXCEPTION("TemplateInfo: uniqueIdUseCount uint16_t overflow!");
	uniqueIdUseCount()[uniqueId-1]++;
	mutex().unlock();
}

TemplateInfo::~TemplateInfo() {
	if (freePointers)
	{
		freeFieldInfo(fieldCount, fieldInfo);
		free(scopeInfo);
		free(dataInfo);
		free(data);
		freePointers = false;
	}
	// decrease reference count in uniqueIdUseCount()
	mutex().lock();
	if((uniqueId == 0) || (uniqueId>uniqueIdUseCount().size()) || (uniqueIdUseCount()[uniqueId-1]==0))
		THROWEXCEPTION("TemplateInfo destructor: uniqueIdUseCount is corrupt (uniqueId=%u, useCount=%u), this should never happen!", uniqueId, uniqueIdUseCount()[uniqueId-1]);
	uniqueIdUseCount()[uniqueId-1]--;
	mutex().unlock();
}

TemplateInfo::FieldInfo *TemplateInfo::allocateFieldInfoArray(size_t numberOfRecords) {
	FieldInfo *array = (FieldInfo *) malloc(sizeof(FieldInfo) * numberOfRecords);

	if (array == NULL)
		return NULL;

	FieldInfo *currentField = array;
	FieldInfo *const endOfArray = array + numberOfRecords;

	while (currentField < endOfArray) {
		currentField->rows = NULL;
		currentField->rowCount = 0;

		currentField++;
	}

	return array;
}

TemplateInfo::StructuredDataRow *TemplateInfo::allocateStructuredDataRowArray(size_t numberOfRecords) {
	StructuredDataRow *array = (StructuredDataRow *) malloc(sizeof(StructuredDataRow) * numberOfRecords);

	if (array == NULL)
		return NULL;

	StructuredDataRow *currentRow = array;
	StructuredDataRow *const endOfArray = array + numberOfRecords;

	while (currentRow < endOfArray) {
		currentRow->fields = NULL;
		currentRow->fieldCount = 0;

		currentRow++;
	}

	return array;
}

void TemplateInfo::freeFieldInfo(uint16_t fieldCount, TemplateInfo::FieldInfo *fields) const {
	TemplateInfo::FieldInfo *currentField = fields;
	TemplateInfo::FieldInfo *const endOfFields = fields + fieldCount;

	while (currentField < endOfFields) {
		if (currentField->type.isStructuredData()) {
			freeStructuredDataRows(currentField->rowCount, currentField->rows);
		}

		currentField++;
	}

	free(fields);
}

void TemplateInfo::freeStructuredDataRows(uint16_t rowCount, TemplateInfo::StructuredDataRow *rows) const {
	if (rows == NULL)
		return;

	TemplateInfo::StructuredDataRow *currentRow = rows;
	TemplateInfo::StructuredDataRow *const endOfRows = rows + rowCount;

	while (currentRow < endOfRows) {
		if (currentRow->fieldCount == 0)
			continue;

		freeFieldInfo(currentRow->fieldCount, currentRow->fields);

		currentRow++;
	}

	free(rows);
}

void TemplateInfo::setUniqueId()
{
	mutex().lock();
	uint16_t oldId = uniqueId;
	uint16_t oldSize = uniqueIdUseCount().size();

	uniqueId = 0;
	for(int i=0; i < oldSize; i++) {
		if(uniqueIdUseCount()[i] == 0) {
			uniqueIdUseCount()[i] = 1;
			uniqueId = i+1; // the id is i+1
			break;
		}
	}
	// if new element was zero, we need to add a new element to uniqueIdUseCount()
	if(uniqueId == 0) {
		if(oldSize == 65535)
			THROWEXCEPTION("TemplateInfo: more than 65353 uniqueIds needed");
		DPRINTF("TemplateInfo: need to increase number of uniqueIds, oldSize=%u", oldSize);
		uniqueIdUseCount().push_back(1);
		uniqueId = oldSize + 1;
	}

	// release old uniqueId if set
	if(oldId != 0) {
		if((oldId > oldSize) || (uniqueIdUseCount()[oldId-1]==0))
			THROWEXCEPTION("TemplateInfo setUniqueId: uniqueIdUseCount is corrupt (oldId=%u, oldSize=%u), this should never happen!", oldId, oldSize);
		uniqueIdUseCount()[oldId-1]--;
	}
	mutex().unlock();
}

/**
 * Gets a Template's FieldInfo by Information Element id. Length is ignored.
 * @param type Information Element to look for. Length is ignored.
 * @return NULL if not found
 */
TemplateInfo::FieldInfo* TemplateInfo::getFieldInfo(const InformationElement::IeInfo& type) {
	return getFieldInfo(type.id, type.enterprise);
}


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
 * @param type Information element Id to look for
 * @return -1 if not found
 */
int TemplateInfo::getFieldIndex(const InformationElement::IeInfo& type) {
	return getFieldIndex(type.id, type.enterprise);
}

/**
 * Gets position of a field in the Template.
 * @param fieldTypeId Information element Id to look for
 * @param fieldTypeEid Enterprise number to look for
 * @return -1 if not found
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

