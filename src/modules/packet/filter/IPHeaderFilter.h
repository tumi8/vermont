/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
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

/*
 * PSAMP Reference Implementation
 *
 * IPHeaderFilter.h
 *
 * Filter by data from IP packet given by (offset, size, comparison, targetvalue)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

/* this is a multi-purpose filter which filters 32-bit values from packets
 example usage:
 a) filter all packets with IP Protocol = 23
 offset=9 (it's located at offset 9)
 size=1 (it's 1 byte)
 comparison=CMP_EQ
 targetvalue=23

 ... you should get the idea
 */

#ifndef IP_HEADER_FILTER_H
#define IP_HEADER_FILTER_H

// required for htons() and htonl()
#include <netinet/in.h>

#include "common/msg.h"

#include "PacketProcessor.h"

// less than
#define CMP_LT  0x00
// less or equal
#define CMP_LE  0x01
// equal
#define CMP_EQ  0x02
// greater or equal
#define CMP_GE  0x03
// greater than
#define CMP_GT  0x04
// not equal
#define CMP_NE  0x05
// Bits are equal -> bitwise AND
#define CMP_BIT 0x06

class IPHeaderFilter : public PacketProcessor
{
public:
	IPHeaderFilter(int header, int offset, int size, int comparison, int value)
		: m_header(header), m_offset(offset), m_size(size), m_comparison(comparison), m_value(value)
	{
		if(!(size==1 || size==2 || size==4)) {
			msg(MSG_ERROR, "invalid size of %d, only 1/2/4 supported", size);
		}
		// check for 2-byte or 4-byte words and convert them from network byte order
		if (size == 2)
		{
		  m_value = htons((unsigned short)(m_value & 0xffff));
		}
		else if (size == 4)
		{
		  m_value = htonl((unsigned long)m_value);
		}
	}

	virtual bool processPacket(Packet *p);

protected:
	bool compareValues(int srcvalue, int dstvalue);
	int getData(void *data, int size);

	/* which header: IP or transport */
	int m_header;

	/* offset from the header above */
	int m_offset;

	/* the size one wants to compare - 1, 2 or 4 bytes */
	int m_size;

	/* the comparison operation, see the CMP_ defines above */
	int m_comparison;

	/* the value to compare against */
	int m_value;
};

#endif

