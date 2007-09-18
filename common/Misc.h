#if !defined(MISC_H)
#define MISC_H

#include <sstream>

using namespace std;


inline string IPToString(uint32_t ip)
{
	ostringstream oss;
	uint8_t* pip = (uint8_t*)(&ip);
	oss << static_cast<int>(pip[0]) << "." << static_cast<int>(pip[1]) << "."
		<< static_cast<int>(pip[2]) << "." << static_cast<int>(pip[3]);
	return oss.str();
}

#endif
