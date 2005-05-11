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

#include "msg.h"

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
        IPHeaderFilter(int offset, int size, int comparison, int value)
                : m_offset(offset), m_size(size), m_comparison(comparison), m_value(value)
        {
        }

        virtual bool processPacket(const Packet *p);

protected:
        bool compareValues(int srcvalue, int dstvalue);
        int getData(void *data, int size);

        int m_offset;
        int m_size;
        int m_comparison;
        int m_value;
};

#endif

