/*
 * PSAMP Reference Implementation
 *
 * Template.h
 *
 * A Template definition
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#ifndef TEMPLATE_H
#define TEMPLATE_H

#include "modules/ipfix/IpfixRecord.hpp"
#include "Packet.h"

#include <stdint.h>


// TODO: Make this dynamic
#define MAX_TEMPLATE_FIELDS 128

// the PSAMP field types used
// (for IPFIX fields the defines from the concentrator are used)
#define PSAMP_TYPEID_ipHeaderPacketSection   313
#define PSAMP_TYPEID_ipPayloadPacketSection   314


class Template
{
	private:
		// the template-id for this template
		uint16_t templateID;
		// the number of fields in this template
		uint16_t fieldCount;

		// the field types for each field
		InformationElement::IeInfo fieldType[MAX_TEMPLATE_FIELDS];

		// the length of each field
		uint16_t fieldLength[MAX_TEMPLATE_FIELDS];

		// the packet classes for which each field is valid. This will
		// be checked against the packet classification before exporting
		uint32_t fieldValidPacketClasses[MAX_TEMPLATE_FIELDS];

	public:
		Template(uint16_t id) : templateID(id), fieldCount(0)
		{
		};

		~Template()
		{
		};

		int getFieldCount() const
		{
			return fieldCount;
		};

		// Add a template field that takes data from within the packet
		void addFieldWithOffset(const InformationElement::IeInfo& type, uint16_t length, uint16_t offset, uint16_t header, uint32_t validPacketClasses)
		{
			DPRINTF("Adding field type %s, length %d, offset %d\n", type.toString().c_str(), length, offset);
			fieldType[fieldCount] = type;
			fieldLength[fieldCount] = length;
			fieldValidPacketClasses[fieldCount] = validPacketClasses;
			fieldCount++;
		};

		// Add a template "meta-"field which gets its data not from the packet itself
		// "length" parameter must still be correct! (i.e. 4 bytes for FT_NUMPACKETS etc.)
		void addFieldWithoutOffset(const InformationElement::IeInfo& type, uint16_t length)
		{
			fieldType[fieldCount] = type;
			fieldLength[fieldCount] = length;
			fieldValidPacketClasses[fieldCount] = (uint32_t)(-1);
			fieldCount++;
		};

		void getFieldInfo(int num, InformationElement::IeInfo* type, uint16_t *length, uint16_t *offset, uint16_t *header) const
		{
			*type = fieldType[num];
			*length = fieldLength[num];
		}

		int getTemplateID() const
		{
			return templateID;
		}

		bool addField(const InformationElement::IeInfo& id, uint16_t len);


		// the supplied packet_classification is check against the requirements of all fields
		// we check whether at least one of the Packet's classification-bits is also set in fieldValidPacketClasses
		bool checkPacketConformity(uint32_t packet_classification)
		{
			unsigned i;
			DPRINTFL(MSG_VDEBUG, "Packet class: %lx", packet_classification);
			for(i=0; i<fieldCount; i++) {
				DPRINTFL(MSG_VDEBUG, "Field id: %s class: %lx", fieldType[i].toString().c_str(), fieldValidPacketClasses[i]);
				// skip meta-fields
				if(!(fieldType[i].enterprise != 0))
					// check if packet class supports this field
					if((packet_classification & fieldValidPacketClasses[i]) == 0)
						return false;
			}
			return true;
		}

		// determine offset, header and packet class for a given field ID
		static bool getFieldOffsetAndHeader(const InformationElement::IeInfo& id, uint16_t *offset, uint16_t *header, uint32_t *validPacketClass);

};

#endif
