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

#define MAX_TEMPLATE_FIELDS 512

// the different field types used
#define FT_SRCIP4   8
#define FT_DSTIP4  12
#define FT_PROTO    4
#define FT_SRCPORT  7
#define FT_DSTPORT 11

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

        void addFieldWithOffset(unsigned short type, unsigned short length, unsigned short offset)
        {
                //DPRINTF("Adding field type %d, length %d, offset %d\n", type, length, offset);
                fieldType[fieldCount] = type;
                fieldLength[fieldCount] = length;
                fieldPacketOffset[fieldCount] = offset;
                fieldCount++;
        };

        void getFieldInfo(int num, unsigned short *type, unsigned short *length, unsigned short *offset) const
        {
                *type = fieldType[num];
                *length = fieldLength[num];
                *offset = fieldPacketOffset[num];
        }

        int getTemplateID() const
        {
                return templateID;
        }

        static Template *readFromFile(const char *fileName);
        bool Template::addField(uint16_t id, uint16_t len);

};

#endif
