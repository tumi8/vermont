#if !defined(MISC_H)
#define MISC_H

#include "ipfixlolib/encoding.h"

#include "common/defs.h"
#include "common/msg.h"

#include <sstream>
#include <string>
#include <ctype.h>

inline std::string IPToString(uint32_t ip)
{
	std::ostringstream oss;
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

/**
 * Implementation of strnlen fro non-GNU systems
 */
#ifndef _GNU_SOURCE
size_t strnlen(const char* s, size_t maxlen);
#endif

inline bool isHexString(const std::string& text)
{
	return text.length()>1 && text[0]=='0' && text[1]=='x';
}

inline char convHexToByte(const char ch)
{
	char c = toupper(ch);
	if (c>='0' && c<='9') return c-'0';
	else if (c>='A' && c<='F') return c-'A';

	THROWEXCEPTION("invalid hexadecimal character: %c", c);
	return 0;  // just to please compiler
}

/**
 * converts given hex string of format '0xAD2382...' to binary
 * @returns length of converted char* array
 */
inline uint32_t convHexToBinary(const std::string& text, char* buffer, uint32_t buflen)
{
	uint32_t len = 0;
	uint32_t pos = 0;

	if (text[0]=='0' && text[1]=='x') pos = 2;

	while (pos<text.length()-1) {
		if (len>=buflen) return len;
		char c1 = convHexToByte(text[pos++]);
		char c2 = convHexToByte(text[pos++]);
		buffer[len++] = (c1<<8) + c2;
	}

	return len;
}

#endif
