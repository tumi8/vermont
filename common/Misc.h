#if !defined(MISC_H)
#define MISC_H

#include "ipfixlolib/encoding.h"

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

/**
 * Returns the sum of two uint32_t values in network byte order
 */
inline uint64_t addUint64Nbo(uint64_t i, uint64_t j) {
	return (htonll(ntohll(i) + ntohll(j)));
}

/**
 * Returns the sum of two uint32_t values in network byte order
 */
inline uint32_t addUint32Nbo(uint32_t i, uint32_t j) {
	return (htonl(ntohl(i) + ntohl(j)));
}

/**
 * Returns the sum of two uint16_t values in network byte order
 */
inline uint16_t addUint16Nbo(uint16_t i, uint16_t j) {
	return (htons(ntohs(i) + ntohs(j)));
}

/**
 * Returns the sum of two uint8_t values in network byte order.
 * As if we needed this...
 */
inline uint8_t addUint8Nbo(uint8_t i, uint8_t j) {
	return (i + j);
}

/**
 * Returns the lesser of two uint32_t values in network byte order
 */
inline uint32_t lesserUint32Nbo(uint32_t i, uint32_t j) {
	return (ntohl(i) < ntohl(j))?(i):(j);
}

/**
 * Returns the greater of two uint32_t values in network byte order
 */
inline uint32_t greaterUint32Nbo(uint32_t i, uint32_t j) {
	return (ntohl(i) > ntohl(j))?(i):(j);
}

/**
 * Returns the lesser of two uint64_t values in network byte order
 */
inline uint64_t lesserUint64Nbo(uint64_t i, uint64_t j) {
	return (ntohll(i) < ntohll(j))?(i):(j);
}

/**
 * Returns the greater of two uint64_t values in network byte order
 */
inline uint64_t greaterUint64Nbo(uint64_t i, uint64_t j) {
	return (ntohll(i) > ntohll(j))?(i):(j);
}


#endif
