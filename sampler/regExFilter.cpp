
#include "regExFilter.h"


inline bool regExFilter::compare(char *pdata)
{

	if (boost::regex_search(pdata, rexp)) {
		return true;
	} 

	return false;

};

bool regExFilter::processPacket(const Packet* p)
{
	const unsigned char* pdata;
	unsigned int plength;
	unsigned int payloadOffset;

	payloadOffset = p->payloadOffset;
	if( payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if(pdata == NULL) return false;

	return compare((char*)pdata);





	return false;

};
