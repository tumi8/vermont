/** @file
 * Filter a packet by checking if it is containing a predefined string
 */

#ifndef STRINGFILTER_H
#define STRINGFILTER_H

#include <vector>
#include <string>
#include "common/msg.h"
#include "PacketProcessor.h"



class stringFilter:public PacketProcessor
{

    public:

	stringFilter ()
	{

	};

	virtual ~ stringFilter ()
	{

	};

	void addandFilter (std::string string)
	{
	    if(string.size()>0)
		andFilters.push_back (string);
	};

	void addnotFilter (std::string string)
	{
	    if(string.size()>0)
		notFilters.push_back (string);
	};

	std::string hexparser(const std::string input);

	virtual bool processPacket (const Packet * p);

	//  int filtertype;


    protected:


	std::vector<std::string> andFilters;
	std::vector<std::string> notFilters;

	bool compare (unsigned char *data, std::string toMatch, unsigned int plength);


	//  char *match;



};

#endif
