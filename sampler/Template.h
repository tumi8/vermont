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

#define MAX_TEMPLATE_FIELDS 512

// the different field types used
#define FT_SRCIP4   8
#define FT_DSTIP4  12
#define FT_PROTO    4
#define FT_SRCPORT  7
#define FT_DSTPORT 11

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
        void addFieldWithOffset(unsigned short type, unsigned short length, unsigned short offset)
        {
                //DPRINTF("Adding field type %d, length %d, offset %d\n", type, length, offset);
                fieldType[fieldCount] = type;
                fieldLength[fieldCount] = length;
                fieldPacketOffset[fieldCount] = offset;
                fieldCount++;
        };

	// Add a template "meta-"field which gets its data not from the packet itself
	// "length" parameter must still be correct! (i.e. 4 bytes for FT_NUMPACKETS etc.)
	void addFieldWithoutOffset(unsigned short type, unsigned short length)
	{
		fieldType[fieldCount] = type;
		fieldLength[fieldCount] = length;
		fieldPacketOffset[fieldCount] = (unsigned short)-1;
		fieldCount++;
	};

        void getFieldInfo(int num, unsigned short *type, unsigned short *length, unsigned short *offset) const
        {
                *type = fieldType[num];
                *length = fieldLength[num];
                *offset = fieldPacketOffset[num];
        }

	// This function returns a temporary buffer with the value of the
	// "meta-" field stored. This buffer must be free()d after use by the user!
	void *getMetaFieldData(int num)
	{
		void *temp;
		
	        switch(fieldType[num])
		{
		case FT_NUMPACKETS:
			// this is for debugging only, caller should ensure it never happens
			// TODO: move this check into addFieldWithoutOffset()
			if (fieldLength[num] < 4)
			{
				DPRINTF("WARNING: Template field %d is too small (4 bytes needed, %d bytes available)",
					num, fieldLength[num]);
				fieldLength[num] = 4;
			}
			
			temp = malloc(fieldLength[num]);
			*((unsigned long *)temp) = Packet::totalPacketsReceived;
			break;
		default:
			return 0;
		}
	}

        int getTemplateID() const
        {
                return templateID;
        }

        static Template *readFromFile(const char *fileName);
        bool Template::addField(uint16_t id, uint16_t len);

};

#endif
