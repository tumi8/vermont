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

};

/**
 * prepare the Packet for comparefucntion
 * set Pointer to Payload
 * @param p Packet data
 * @return true if packet contains string false otherwise
 */
bool StringFilter::processPacket(const Packet *p)
{
    unsigned char* pdata;
    unsigned int plength;
    unsigned int payloadOffset;
    std::vector<std::string>::iterator iti;

    payloadOffset = p->payloadOffset;
    if( payloadOffset == 0) return false;
    pdata = (unsigned char*)p->data + payloadOffset;
    plength = p->data_length - payloadOffset;

    if(pdata == NULL) return false;

    for(iti = andFilters.begin(); iti != andFilters.end(); ++iti)
	if(!compare(pdata, *iti, plength)) 
	    return false;

    for(iti = notFilters.begin(); iti != notFilters.end(); ++iti)
	if(compare(pdata, *iti, plength)) 
	    return false;

    return true;
};
