
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
	bool result;

	payloadOffset = p->payloadOffset;
	if( payloadOffset == 0) return false;
	pdata = p->data + payloadOffset;
	plength = p->data_length - payloadOffset;

	if(pdata == NULL) return false;

	result = compare((char*)pdata);

    if(result) {
    char cmd[512];
    sprintf(cmd,"/usr/bin/perl -I/monitor/falko/ims_idmefsender/includes /monitor/falko/ims_idmefsender/ims_idmefsender.pl %d.%d.%d.%d %d.%d.%d.%d regexfilter",p->netHeader[12],p->netHeader[13],p->netHeader[14],p->netHeader[15],p->netHeader[16],p->netHeader[17],p->netHeader[18],p->netHeader[19]);
    system(cmd);
    }



	return result;

};
