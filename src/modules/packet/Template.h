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

#include "common/msg.h"


// TODO: Make this dynamic
#define MAX_TEMPLATE_FIELDS 128

class Template
{
	private:
		// the template-id for this template
		uint16_t templateID;
		// the number of fields in this template
		uint16_t fieldCount;

		// the field types for each field
		InformationElement::IeInfo fieldType[MAX_TEMPLATE_FIELDS];

		// the offset in the TCP/IP packet for each field
		uint16_t fieldPacketOffset[MAX_TEMPLATE_FIELDS];

		// the header from which the corresponding field is taken
		// HEAD_RAW, HEAD_NETWORK,... (see Packet.h)
		uint16_t fieldPacketHeader[MAX_TEMPLATE_FIELDS];

		// the packet classes for which each field is valid. This will
		// be checked against the packet classification before exporting
        uint32_t  fieldValidPacketClasses[MAX_TEMPLATE_FIELDS];

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
		void addFieldWithOffset(const InformationElement::IeInfo& ie, uint16_t offset, uint16_t header, uint32_t validPacketClasses)
		{
            // check if this template field can be accepted
            if (fieldCount < MAX_TEMPLATE_FIELDS)
            {
                fieldType[fieldCount] = ie;
                fieldPacketOffset[fieldCount] = offset;
                fieldPacketHeader[fieldCount] = header;
                fieldValidPacketClasses[fieldCount] = validPacketClasses;
                fieldCount++;
            }
            else
            {
                // the maximum number of template fields has been exceeded -> abort
                THROWEXCEPTION("exceeded maximum number of template fields (%d)", MAX_TEMPLATE_FIELDS);
            }
		};

		// Add a template "meta-"field which gets its data not from the captured packet itself
		// "length" parameter must still be correct! (i.e. 4 bytes for FT_NUMPACKETS etc.)
		void addFieldWithoutOffset(const InformationElement::IeInfo& ie)
		{
            // check if this template field can be accepted
            if (fieldCount < MAX_TEMPLATE_FIELDS)
            {
                fieldType[fieldCount] = ie;
                fieldValidPacketClasses[fieldCount] = (uint32_t)(-1);
                fieldCount++;
            }
            else
            {
                // the maximum number of template fields has been exceeded -> abort
                THROWEXCEPTION("exceeded maximum number of template fields (%d)", MAX_TEMPLATE_FIELDS);
            }
		};

		void getFieldInfo(int num, InformationElement::IeInfo* ie, uint16_t* offset, uint16_t* header) const
        {
            if (num < fieldCount)
            {
                *ie = fieldType[num];
                *offset = fieldPacketOffset[num];
                *header = fieldPacketHeader[num];
            }
            else
            {
                // template field not initialized -> abort
                THROWEXCEPTION("template field %d is not initialized", num);
            }
		}

		int getTemplateID() const
		{
			return templateID;
		}

		bool addField(const InformationElement::IeInfo& ie);


		// the supplied packet_classification is check against the requirements of all fields
		// we check whether at least one of the Packet's classification-bits is also set in fieldValidPacketClasses
		bool checkPacketConformity(uint32_t packet_classification)
		{
			unsigned i;
			DPRINTFL(MSG_VDEBUG, "Packet class: %lx", packet_classification);
			for(i=0; i<fieldCount; i++) {
				DPRINTFL(MSG_VDEBUG, "Field id: %s class: %lx", fieldType[i].toString().c_str(), fieldValidPacketClasses[i]);
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
