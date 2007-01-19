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

#include <stdint.h>
#include "Packet.h"

// TODO: Make this dynamic
#define MAX_TEMPLATE_FIELDS 128

// the PSAMP field types used
// (for IPFIX fields the defines from the concentrator are used)
#define PSAMP_TYPEID_ipHeaderPacketSection   313
#define PSAMP_TYPEID_ipPayloadPacketSection   314

// below are "meta"-fields, i.e. field types which
// do not get their data from the packet itself
// all of these should be >= 0x8000
#define FT_NUMPACKETS 0x8001    // number of packets received up to now. DWORD.


class Template
{
private:
        // the template-id for this template
        unsigned short templateID;
        // the number of fields in this template
        unsigned short fieldCount;

        // the field types for each field
        unsigned short fieldType[MAX_TEMPLATE_FIELDS];

        // the length of each field
        unsigned short fieldLength[MAX_TEMPLATE_FIELDS];

        // the offset in the TCP/IP packet for each field
        unsigned short fieldPacketOffset[MAX_TEMPLATE_FIELDS];

	// the header from which the corresponding field is taken
	// 0 = RAW packet, 1 = IP header, 2 = TCP header
	// -1 for metadata field
	unsigned short fieldPacketHeader[MAX_TEMPLATE_FIELDS];

	// the packet classes for which each field is valid. This will
	// be checked against the packet classification before exporting
	unsigned long fieldValidPacketClasses[MAX_TEMPLATE_FIELDS];

	// determin offset, header and packet class for a given field ID
	// if needed, make this method public
	bool getFieldOffsetAndHeader(uint16_t id, uint16_t *offset, unsigned short *header, unsigned long *validPacketClass);
	
	
public:
        Template(unsigned short id) : templateID(id), fieldCount(0)
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
        void addFieldWithOffset(unsigned short type, unsigned short length, unsigned short offset, unsigned short header, unsigned long validPacketClasses)
        {
                DPRINTF("Adding field type %d, length %d, offset %d\n", type, length, offset);
                fieldType[fieldCount] = type;
                fieldLength[fieldCount] = length;
                fieldPacketOffset[fieldCount] = offset;
		fieldPacketHeader[fieldCount] = header;
		fieldValidPacketClasses[fieldCount] = validPacketClasses;
                fieldCount++;
        };

	// Add a template "meta-"field which gets its data not from the packet itself
	// "length" parameter must still be correct! (i.e. 4 bytes for FT_NUMPACKETS etc.)
	void addFieldWithoutOffset(unsigned short type, unsigned short length)
	{
		fieldType[fieldCount] = type;
		fieldLength[fieldCount] = length;
		fieldPacketOffset[fieldCount] = (unsigned short)(-1);
		fieldPacketHeader[fieldCount] = (unsigned short)(-1);
		fieldValidPacketClasses[fieldCount] = (unsigned long)(-1);
		fieldCount++;
	};

        void getFieldInfo(int num, unsigned short *type, unsigned short *length, unsigned short *offset, unsigned short *header) const
        {
                *type = fieldType[num];
                *length = fieldLength[num];
                *offset = fieldPacketOffset[num];
		*header = fieldPacketHeader[num];
        }

	// This function returns a temporary buffer with the value of the
	// "meta-" field stored. This buffer must be free()d after use by the user!
	void *getMetaFieldData(int num)
	{
		void *temp;
		
		switch(fieldType[num]) {
		case FT_NUMPACKETS:
			// this is for debugging only, caller should ensure it never happens
			// TODO: move this check into addFieldWithoutOffset()
			if(fieldLength[num] < 4) {
				DPRINTF("WARNING: Template field %d is too small (4 bytes needed, %d bytes available)",
					num, fieldLength[num]);
				fieldLength[num] = 4;
			}

			temp = malloc(fieldLength[num]);
			*((unsigned long *)temp) = Packet::totalPacketsReceived;
			return temp;
		default:
			return 0;
		}
	}

        int getTemplateID() const
        {
                return templateID;
        }

/* This is not used in vermont */
#if 0
        static Template *readFromFile(const char *fileName);
#endif	
        bool addField(uint16_t id, uint16_t len);

	
	// the supplied packet_classification is check against the requirements of all fields
	// we check whether at least one of the Packet's classification-bits is also set in fieldValidPacketClasses
	bool checkPacketConformity(unsigned long packet_classification)
	{
	    unsigned i;
	    DPRINTF("Packet class: %lx\n", packet_classification);
	    for(i=0; i<fieldCount; i++) {
		DPRINTF("Field id: %u class: %lx\n", fieldType[i], fieldValidPacketClasses[i]);
		// skip meta-fields
		if(!(fieldType[i] > 0x8000))
		    // check if packet class supports this field
		    if((packet_classification & fieldValidPacketClasses[i]) == 0)
			return false;
	    }
	    return true;
	}
	    

};

#endif
