/*
 * PSAMP Reference Implementation
 *
 * Template.cpp
 *
 * A Template definition
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include <cstring>
#include <fstream>
#include <iostream>

#include "msg.h"

#include "Template.h"

using namespace std;


bool Template::addField(uint16_t id, uint16_t len)
{
        uint16_t offset;

	/* it is a field with data from the packet itself */
	if(id < 0x8000) {
		switch(id) {
	        case FT_SRCIP4:
	                offset=12;
	                break;
	        case FT_DSTIP4:
	                offset=16;
	                break;
	        case FT_PROTO:
	                offset=9;
	                break;
	        case FT_SRCPORT:
	                offset=20;
	                break;
	        case FT_DSTPORT:
	                offset=22;
	                break;
	        default:
	                msg(MSG_ERROR, "ID %d currently not supported", id);
	                return false;
		}

		addFieldWithOffset(id, len, offset);
        } else {
		addFieldWithoutOffset(id, len);
	}
	
        return true;
}


/* this is not needed within vermont */
#if 0
void AddFieldFromString(Template *temp, const char *field)
{
        if(strncasecmp(field, "SRCIP4", 6) == 0) {
                // source address is as offset 12
                temp->addFieldWithOffset(FT_SRCIP4, 4, 12);
        } else if(strncasecmp(field, "DSTIP4", 6) == 0) {
                // dest address is at offset 16
                temp->addFieldWithOffset(FT_DSTIP4, 4, 16);
        } else if(strncasecmp(field, "PROTO", 5) == 0) {
                // protocol is at offset 9
                temp->addFieldWithOffset(FT_PROTO, 2, 9);
        } else if(strncasecmp(field, "SRCPORT", 7) == 0) {
                // source port is as offset 20 (TCP offset 0)
                temp->addFieldWithOffset(FT_SRCPORT, 2, 20);
        } else if(strncasecmp(field, "DSTPORT", 7) == 0) {
                // dest port is at offset 22 (TCP offset 2)
                temp->addFieldWithOffset(FT_DSTPORT, 2, 22);
        } else if(strncasecmp(field, "NUMPACKETS", 10) == 0) {
		// number of pacets received. This is metadata!
		temp->addFieldWithoutOffset(FT_NUMPACKETS, 4);
        }
}

Template *Template::readFromFile(const char *fileName)
{
        char buffer[256];
        Template *tmp = 0;

        ifstream f(fileName);

        // get template id
        while (!f.eof())
        {
                f.getline(buffer, 255);
                if ((buffer[0] == '#') || (buffer[0] == 0x0d) || (buffer[0] == 0x0a) || (buffer[0] == 0))
                        continue;

                if (strncasecmp(buffer, "ID ", 3) == 0)
                {
                        // assign template id
                        tmp = new Template(strtol(buffer + 3, 0, 10));
                        break;
                }
                else
                {
                        DPRINTF("Expected ID\n");
                        return 0;
                }
        }

        // get template fields
        while (!f.eof())
        {
                f.getline(buffer, 255);
                if ((buffer[0] == '#') || (buffer[0] == 0x0d) || (buffer[0] == 0x0a) || (buffer[0] == 0))
                        continue;

                AddFieldFromString(tmp, buffer);
        }

        f.close();

        return tmp;
}
#endif
