/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

/*
 * Internal representation of an IP address: up to 5 bytes: 192.168.0.201/24 ==> 192 168 0 201 8(!)
 * Internal representation of a Port range: 2n bytes: 80,443 ==> 80 80 443 443
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "Rules.hpp"
#include "common/ipfixlolib/ipfix.h"

#include "common/msg.h"
#include "modules/ipfix/Connection.h"

#define MAX_LINE_LEN 256

/* --- constants ------------*/


/* --- functions ------------*/

/**
 * Checks whether a given letter is found in an alphabet or not
 * @return 1 if @c letter is found
 */
int is_in(char letter, const char* alphabet) {
	int i;
	for (i = 0; i < (int)strlen(alphabet); i++) {
		if (alphabet[i] == letter) return 1;
	}
	return 0;
}

#if 0
/* defined but not used */
/**
 * Removes whitespace at the end of a string by overwriting it with 0-bytes
 */
void rtrim(char* text) {
	while ((*text != 0) && is_in(text[strlen(text)-1], " \n\t")) text[strlen(text)-1] = 0;
}

/**
 * Removes whitespace at the start of a string by returning a pointer with an added offset
 */
char* ltrim(char* text) {
	while ((*text != 0) && is_in(*text, " \n\t")) ++text;
	return text;
}
#endif

/**
 * Successively tokenizes a string by inserting 0-bytes, returning pointers to the tokens one at a time.
 * Multiple delimiters between tokens are treated as one.
 * Modifies the source string!
 * @param text pointerpointer to the string to be tokenized. Its start-pointer will be shifted to reflect the current position while tokenizing
 * @param delim string of delimiters that seperate tokens
 * @return pointer to first token. Call again to retrieve next token
 */
char* get_next_token(char** text, const char* delim) {
	char* p = *text;

	if (**text == 0) return NULL;

	for (; **text != 0; ++*text) {
		if (is_in(**text, delim)) {
			**text = 0; ++*text;
			while ((**text != 0) && (is_in(**text, delim))) {
				++*text;
			}
			break;
		}
	}
	return p;
}

/**
 * parses the given string
 * @return 0 if successful
 */
int parseModifier(const char* s, Rule::Field::Modifier* modifier) {
	if (strcmp(s, "discard") == 0) { *modifier = Rule::Field::DISCARD; return 0; }
	if (strcmp(s, "keep") == 0) { *modifier = Rule::Field::KEEP; return 0; }
	if (strcmp(s, "aggregate") == 0) { *modifier = Rule::Field::AGGREGATE; return 0; }
	if (strncmp(s, "mask/", 5) == 0) { *modifier = (Rule::Field::Modifier)((int)Rule::Field::MASK_START + atoi(s+5)); return 0; }
	return -1;
}

/**
 * parses the given string
 * @return 0 if successful
 */
int parseProtoPattern(const char* s, IpfixRecord::Data** fdata, InformationElement::IeLength* length) {
	int proto = -1;
	if (strcmp(s, "ICMP") == 0) proto = IPFIX_protocolIdentifier_ICMP;
	if (strcmp(s, "TCP") == 0) proto = IPFIX_protocolIdentifier_TCP;
	if (strcmp(s, "UDP") == 0) proto = IPFIX_protocolIdentifier_UDP;
	if (strcmp(s, "RAW") == 0) proto = IPFIX_protocolIdentifier_RAW;
	if (strcmp(s, "SCTP") == 0) proto = IPFIX_protocolIdentifier_SCTP;

	if (proto == -1)
	{
		char* end;
		errno = 0;
		proto = strtol(s, &end, 10);
		if(end == s || *end != 0 || errno != 0 || (proto < 0) || (proto > 255)) return -1;
	}

	*length = 1;
	IpfixRecord::Data* fd = (IpfixRecord::Data*)malloc(*length);
	fd[0] = proto;
	*fdata = fd;

	return 0;
}

/**
 * parses the given string
 * @return 0 if successful
 */
int parseIPv4Pattern(char* s, IpfixRecord::Data** fdata, InformationElement::IeLength* length) {
	char* p = s;
	char* octet1 = get_next_token(&p, ".");
	char* octet2 = get_next_token(&p, ".");
	char* octet3 = get_next_token(&p, ".");
	char* octet4 = get_next_token(&p, "/");
	char* imask = get_next_token(&p, " ");
	if (!octet4) return -1;
/*
	if (imask) {
		*length = 5;
	} else {
		*length = 4;
	}
*/
	*length = 5;

	IpfixRecord::Data* fd = (IpfixRecord::Data*)malloc(*length);
	fd[0]=atoi(octet1);
	fd[1]=atoi(octet2);
	fd[2]=atoi(octet3);
	fd[3]=atoi(octet4);
	fd[4]=0;
	if (imask) fd[4]=32-atoi(imask);
	*fdata = fd;
	return 0;
}

/**
 * parses the given string
 * @return 0 if successful
 */
int parsePortPattern(char* s, IpfixRecord::Data** fdata, InformationElement::IeLength* length) {
	char buf[256];

	char* p = s;
	*length = 0;
	*fdata = 0;
	char* pair;

	while ((pair = get_next_token(&p, ","))) {
		strncpy(buf, pair, sizeof(buf)-1);
		char* p2 = buf;
		char* start = get_next_token(&p2, ":");
		char* end = get_next_token(&p2, ",");
		*length = *length + 4;
		*fdata = (IpfixRecord::Data*)realloc(*fdata, *length);
		uint16_t starti = atoi(start);
		uint16_t endi = starti;
		if (end) endi = atoi(end);

		(*fdata)[*length-4] = ((starti >> 8) & 0xFF);
		(*fdata)[*length-3] = ((starti >> 0) & 0xFF);
		(*fdata)[*length-2] = ((endi >> 8) & 0xFF);
		(*fdata)[*length-1] = ((endi >> 0) & 0xFF);
	}

	return 0;
}

/**
 * parses the given string
 * @return 0 if successful
 */
int parseTcpFlags(char* s, IpfixRecord::Data** fdata, InformationElement::IeLength* length) {
	uint16_t flags = 0;

	char* p = s;
	char* pair;

	while ((pair = get_next_token(&p, ","))) {
		if (strcmp(pair, "FIN") == 0) flags = flags | Connection::FIN;
		else if (strcmp(pair, "SYN") == 0) flags = flags | Connection::SYN;
		else if (strcmp(pair, "RST") == 0) flags = flags | Connection::RST;
		else if (strcmp(pair, "PSH") == 0) flags = flags | Connection::PSH;
		else if (strcmp(pair, "ACK") == 0) flags = flags | Connection::ACK;
		else if (strcmp(pair, "URG") == 0) flags = flags | Connection::URG;
		else if (strcmp(pair, "ECE") == 0) flags = flags | Connection::ECE;
		else if (strcmp(pair, "CWR") == 0) flags = flags | Connection::CWR;
		else if (strcmp(pair, "NS") == 0) flags = flags | Connection::NS;
		else return -1;
	}


	*length = 2;
	IpfixRecord::Data* fd = (IpfixRecord::Data*)malloc(*length);
	fd[0] = flags;
	*fdata = fd;

	return 0;
}

/**
 * De-allocates memory used by rules.
 * This will NOT destroy hashtables associated with individual rules
 */
Rules::~Rules() {
	for (size_t i = 0; i < count; i++) {
		delete rule[i];
	}
}

/**
 * Creates an empty set of rules
 */
Rules::Rules() : count(0) {
}

bool operator==(const Rules &rhs, const Rules &lhs) {
	if (rhs.count != lhs.count) return false;

	for (size_t i = 0; i < rhs.count; i++) {
		if (*rhs.rule[i] != *lhs.rule[i]) return false;
	}

	return true;
}

bool operator!=(const Rules &rhs, const Rules &lhs) {
	return !(lhs == rhs);
}
