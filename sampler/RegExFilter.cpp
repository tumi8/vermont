
#include "RegExFilter.h"


inline bool RegExFilter::compare(char *pdata)
{

	if (boost::regex_search(pdata, rexp)) {
		return true;
	} 

	return false;

};

bool RegExFilter::processPacket(Packet* p)
{
	const unsigned char* pdata;
	unsigned int plength;
	unsigned int payloadOffset;
	bool result;

	payloadOffset = p->payloadOffset;
	if( payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if(pdata == NULL) return false;

	result = compare((char*)pdata);

	return result;

};
