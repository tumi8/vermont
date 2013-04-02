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
 * IPHeaderFilter.cpp
 *
 * Filter by data from IP packet given by (offset, size, comparison, targetvalue)
 *
 * Author: Michael Drueing <michael@drueing.de>
 *
 */

#include "IPHeaderFilter.h"

inline bool IPHeaderFilter::compareValues(int srcvalue, int dstvalue)
{
        switch(m_comparison){
        case CMP_LT:
                return srcvalue < dstvalue;
        case CMP_LE:
                return srcvalue <= dstvalue;
        case CMP_EQ:
                return srcvalue == dstvalue;
        case CMP_GE:
                return srcvalue >= dstvalue;
        case CMP_GT:
                return srcvalue > dstvalue;
        case CMP_NE:
                return srcvalue != dstvalue;
        case CMP_BIT:
//                return ((srcvalue & dstvalue) == dstvalue);
	/* muenz: match if at least one bit of the mask is set? */
                return ((srcvalue & dstvalue) != 0);
        default:
                return 0;
        }
}

inline int IPHeaderFilter::getData(void *data, int size)
{
        switch(size){
        case 1:
                return *((unsigned char *)data);
        case 2:
                return *((unsigned short *)data);
        case 4:
                return *((int *)data);
        default:
                msg(MSG_ERROR, "invalid Data Size %d", size);
                return 0;
        }
}

bool IPHeaderFilter::processPacket(Packet *p)
{
        int srcvalue;
        void *start;

        switch(m_header) {
        case 1:
                start=p->data.netHeader;
                break;
        case 2:
                start=p->transportHeader;
                break;
        default:
                start=p->data.netHeader;
        }

	if(start == NULL)
	    return false;

        srcvalue=getData(((char*)start + m_offset), m_size);
        return compareValues(srcvalue, m_value);
}

