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

/** \file
 * This is the String Filter
 */


#include "StringFilter.h"


StringFilter::StringFilter()
{
}

StringFilter::~StringFilter()
{
}

void StringFilter::addandFilter(std::string string)
{
    if(string.size()>0)
	andFilters.push_back (string);
}

void StringFilter::addnotFilter(std::string string)
{
    if(string.size()>0)
	notFilters.push_back (string);
}

std::string StringFilter::hexparser(const std::string input) 
{
    unsigned int i;
    std::string output("");
    char hex[5];

    for (i = 0; i < input.size(); i += 2)
    {
	hex[0] = '0';
	hex[1] = 'x';
	hex[2] = input[i];
	hex[3] = input[i + 1];
	hex[4] = 0;
	output.append(1, (char)strtol(hex, NULL, 16));
    }

    return output;
}

/**
 * compare given String to packet data and check for a match
 * @param pdata Packet payload
 * @param toMatch the String to be matched against the packet
 * @param plength The packetlength
 * @return true on match false otherwise
 */
inline bool StringFilter::compare(unsigned char *pdata, std::string toMatch, unsigned int plength)
{
    unsigned counter=0;
    char* tmp;
    unsigned i=0;

    while(i + toMatch.size() <=  plength) 
    {
	if ((char)pdata[i] == toMatch[0]) 
	{
	    counter = 1;
	    tmp = (char*)&pdata[i];
	    for( unsigned int j=1; j < toMatch.size(); j++) 
	    {
		if(tmp[j] != toMatch[j]) 
		    break;
		counter++;
	    }
	    if(counter == toMatch.size())
		return true;
	}
	i++;
    }
    return false;

}

/**
 * prepare the Packet for comparefucntion
 * set Pointer to Payload
 * @param p Packet data
 * @return true if packet contains string false otherwise
 */
bool StringFilter::processPacket(Packet *p)
{
    unsigned char* pdata;
    unsigned int plength;
    unsigned int payloadOffset;
    std::vector<std::string>::iterator iti;

    payloadOffset = p->payloadOffset;
    if( payloadOffset == 0) return false;
    pdata = (unsigned char*)p->data.netHeader + payloadOffset;
    plength = p->data_length - payloadOffset;

    if(pdata == NULL) return false;

    for(iti = andFilters.begin(); iti != andFilters.end(); ++iti)
	if(!compare(pdata, *iti, plength)) 
	    return false;

    for(iti = notFilters.begin(); iti != notFilters.end(); ++iti)
	if(compare(pdata, *iti, plength)) 
	    return false;

    return true;
}
