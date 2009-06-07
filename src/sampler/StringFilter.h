/** @file
 * Filter a packet by checking if it is containing a predefined string
 */

#ifndef STRINGFILTER_H
#define STRINGFILTER_H

#include <vector>
#include <string>
#include "common/msg.h"
#include "PacketProcessor.h"



class StringFilter : public PacketProcessor
{
public:
	StringFilter();
	virtual ~StringFilter();

	static std::string hexparser(const std::string input);
	virtual bool processPacket (Packet * p);
	void addandFilter(std::string string);
	void addnotFilter(std::string string);

protected:
	std::vector<std::string> andFilters;
	std::vector<std::string> notFilters;

	bool compare (unsigned char *data, std::string toMatch, unsigned int plength);
};

#endif
