#include <string.h>
#include <stdio.h>
#include "ipfix_names.h"
#include "concentrator/ipfix.hpp"

#ifdef __cplusplus
extern "C" {
#endif

static const struct ipfix_identifier IPFIXTAB[] = { { "reserved", 0, 0 },
		{ "octetdeltacount", 1, 8 }, { "packetdeltacount", 2, 8 }, { "reserved", 3, 0 }, {
				"protocolidentifier", 4, 1 }, { "classofserviceipv4", 5, 1 }, { "tcpcontrolbits",
				6, 1 }, { "sourcetransportport", 7, 2 }, { "sourceipv4address", 8, 4 }, {
				"sourceipv4mask", 9, 1 }, { "ingressinterface", 10, 4 }, {
				"destinationtransportport", 11, 2 }, { "destinationipv4address", 12, 4 }, {
				"destinationipv4mask", 13, 1 }, { "egressinterface", 14, 4 }, {
				"ipnexthopipv4address", 15, 4 }, { "bgpsourceasnumber", 16, 2 }, {
				"bgpdestinationasnumber", 17, 2 }, { "bgpnexthopipv4address", 18, 4 }, {
				"postmcastpacketdeltacount", 19, 8 }, { "postmcastoctetdeltacount", 20, 8 }, {
				"flowendsysuptime", 21, 4 }, { "flowstartsysuptime", 22, 4 }, {
				"postoctetdeltacount", 23, 8 }, { "postpacketdeltacount", 24, 8 }, {
				"minimumpacketlength", 25, 2 }, { "maximumpacketlength", 26, 2 }, {
				"sourceipv6address", 27, 8 }, { "destinationipv6address", 28, 8 }, {
				"sourceipv6mask", 29, 1 }, { "destinationipv6mask", 30, 1 }, { "flowlabelipv6", 31,
				4 }, { "icmptypecodeipv4", 32, 2 }, { "igmptype", 33, 1 }, { "reserved", 34, 0 }, {
				"reserved", 35, 0 }, { "flowactivetimeout", 36, 2 },
		{ "flowinactivetimeout", 37, 2 }, { "reserved", 38, 0 }, { "reserved", 39, 0 }, {
				"exportedoctettotalcount", 40, 8 }, { "exportedmessagetotalcount", 41, 8 }, {
				"exportedflowtotalcount", 42, 8 }, { "reserved", 43, 0 }, { "sourceipv4prefix", 44,
				4 }, { "destinationipv4prefix", 45, 4 }, { "mplstoplabeltype", 46, 1 }, {
				"mplstoplabelipv4address", 47, 4 }, { "reserved", 48, 0 }, { "reserved", 49, 0 }, {
				"reserved", 50, 0 }, { "reserved", 51, 0 }, { "minimumttl", 52, 1 }, {
				"maximumttl", 53, 1 }, { "identificationipv4", 54, 2 }, { "postclassofserviceipv4",
				55, 1 }, { "sourcemacaddress", 56, 0 }, { "postdestinationmacaddress", 57, 0 }, {
				"vlanid", 58, 2 }, { "postvlanid", 59, 2 }, { "ipversion", 60, 1 }, { "reserved",
				61, 0 }, { "ipnexthopipv6address", 62, 8 }, { "bgpnexthopipv6address", 63, 8 }, {
				"ipv6extensionheaders", 64, 4 }, { "reserved", 65, 0 }, { "reserved", 66, 0 }, {
				"reserved", 67, 0 }, { "reserved", 68, 0 }, { "reserved", 69, 0 }, {
				"mplstoplabelstackentry", 70, 4 }, { "mplslabelstackentry2", 71, 4 }, {
				"mplslabelstackentry3", 72, 4 }, { "mplslabelstackentry4", 73, 4 }, {
				"mplslabelstackentry5", 74, 4 }, { "mplslabelstackentry6", 75, 4 }, {
				"mplslabelstackentry7", 76, 4 }, { "mplslabelstackentry8", 77, 4 }, {
				"mplslabelstackentry9", 78, 4 }, { "mplslabelstackentry10", 79, 4 }, {
				"destinationmacaddress", 80, 0 }, { "postsourcemacaddress", 81, 0 }, { "reserved",
				82, 0 }, { "reserved", 83, 0 }, { "reserved", 84, 0 },
		{ "octettotalcount", 85, 8 }, { "packettotalcount", 86, 8 }, { "reserved", 87, 0 }, {
				"fragmentoffsetipv4", 88, 2 }, { "reserved", 89, 0 }, { "reserved", 90, 0 }, {
				"reserved", 91, 0 }, { "reserved", 92, 0 }, { "reserved", 93, 0 }, { "reserved",
				94, 0 }, { "reserved", 95, 0 }, { "reserved", 96, 0 }, { "reserved", 97, 0 }, {
				"reserved", 98, 0 }, { "reserved", 99, 0 }, { "reserved", 100, 0 }, { "reserved",
				101, 0 }, { "reserved", 102, 0 }, { "reserved", 103, 0 }, { "reserved", 104, 0 }, {
				"reserved", 105, 0 }, { "reserved", 106, 0 }, { "reserved", 107, 0 }, { "reserved",
				108, 0 }, { "reserved", 109, 0 }, { "reserved", 110, 0 }, { "reserved", 111, 0 }, {
				"reserved", 112, 0 }, { "reserved", 113, 0 }, { "reserved", 114, 0 }, { "reserved",
				115, 0 }, { "reserved", 116, 0 }, { "reserved", 117, 0 }, { "reserved", 118, 0 }, {
				"reserved", 119, 0 }, { "reserved", 120, 0 }, { "reserved", 121, 0 }, { "reserved",
				122, 0 }, { "reserved", 123, 0 }, { "reserved", 124, 0 }, { "reserved", 125, 0 }, {
				"reserved", 126, 0 }, { "reserved", 127, 0 },
		{ "bgpnextadjacentasnumber", 128, 2 }, { "bgpprevadjacentasnumber", 129, 2 }, {
				"exporteripv4address", 130, 4 }, { "exporteripv6address", 131, 8 }, {
				"droppedoctetdeltacount", 132, 8 }, { "droppedpacketdeltacount", 133, 8 }, {
				"droppedoctettotalcount", 134, 8 }, { "droppedpackettotalcount", 135, 8 }, {
				"flowendreason", 136, 1 }, { "classofserviceipv6", 137, 1 }, {
				"postclassofserviceipv6", 138, 1 }, { "icmptypecodeipv6", 139, 2 }, {
				"mplstoplabelipv6address", 140, 8 }, { "linecardid", 141, 4 },
		{ "portid", 142, 4 }, { "meteringprocessid", 143, 4 }, { "exportingprocessid", 144, 4 }, {
				"templateid", 145, 2 }, { "wlanchannelid", 146, 1 }, { "wlanssid", 147, 0 }, {
				"flowid", 148, 4 }, { "sourceid", 149, 4 }, { "flowstartseconds", 150, 4 }, {
				"flowendseconds", 151, 4 }, { "flowstartmilliseconds", 152, 8 }, {
				"flowendmilliseconds", 153, 8 }, { "flowstartmicroseconds", 154, 4 }, {
				"flowendmicroseconds", 155, 4 }, { "flowstartnanoseconds", 156, 4 }, {
				"flowendnanoseconds", 157, 4 }, { "flowstartdeltamicroseconds", 158, 4 }, {
				"flowenddeltamicroseconds", 159, 4 }, { "systeminittimemilliseconds", 160, 8 }, {
				"flowdurationmilliseconds", 161, 8 }, { "flowdurationmicroseconds", 162, 4 }, {
				"observedflowtotalcount", 163, 8 }, { "ignoredpackettotalcount", 164, 8 }, {
				"ignoredoctettotalcount", 165, 8 }, { "notsentflowtotalcount", 166, 8 }, {
				"notsentpackettotalcount", 167, 8 }, { "notsentoctettotalcount", 168, 8 }, {
				"destinationipv6prefix", 169, 8 }, { "sourceipv6prefix", 170, 8 }, {
				"postoctettotalcount", 171, 8 }, { "postpackettotalcount", 172, 8 }, {
				"flowkeyindicator", 173, 8 }, { "postmcastpackettotalcount", 174, 8 }, {
				"postmcastoctettotalcount", 175, 8 }, { "icmptypeipv4", 176, 1 }, { "icmpcodeipv4",
				177, 1 }, { "icmptypeipv6", 178, 1 }, { "icmpcodeipv6", 179, 1 }, {
				"udpsourceport", 180, 2 }, { "udpdestinationport", 181, 2 }, { "tcpsourceport",
				182, 2 }, { "tcpdestinationport", 183, 2 }, { "tcpsequencenumber", 184, 4 }, {
				"tcpacknowledgementnumber", 185, 4 }, { "tcpwindowsize", 186, 2 }, {
				"tcpurgentpointer", 187, 2 }, { "tcpheaderlength", 188, 2 }, { "ipheaderlength",
				189, 1 }, { "totallengthipv4", 190, 2 }, { "payloadlengthipv6", 191, 4 }, {
				"iptimetolive", 192, 1 }, { "nextheaderipv6", 193, 1 }, { "ipclassofservice", 194,
				1 }, { "ipdiffservcodepoint", 195, 1 }, { "ipprecedence", 196, 1 }, {
				"fragmentflagsipv4", 197, 1 }, { "octetdeltasumofsquares", 198, 8 }, {
				"octettotalsumofsquares", 199, 8 }, { "mplstoplabelttl", 200, 1 }, {
				"mplslabelstacklength", 201, 4 }, { "mplslabelstackdepth", 202, 4 }, {
				"mplstoplabelexp", 203, 1 }, { "ippayloadlength", 204, 8 }, { "udpmessagelength",
				205, 2 }, { "ismulticast", 206, 1 }, { "internetheaderlengthipv4", 207, 1 }, {
				"ipv4options", 208, 4 }, { "tcpoptions", 209, 8 }, { "paddingoctets", 210, 0 }, {
				"reserved", 211, 0 }, { "reserved", 212, 0 }, { "headerlengthipv4", 213, 1 }, {
				"mplspayloadlength", 214, 4 }, { "selectorid", 300, 2 }, {
				"selectorinputsequencenumber", 301, 4 }, { "selectoralgorithm", 302, 1 }, {
				"reserved", 303, 0 }, { "samplingpacketinterval", 304, 4 }, {
				"samplingpacketspace", 305, 4 }, { "samplingtimeinterval", 306, 4 }, {
				"samplingtimespace", 307, 4 }, { "samplingpopulation", 308, 4 }, { "samplingsize",
				309, 4 }, { "samplingprobabilityn", 310, 4 }, { "samplingprobabilitym", 311, 4 }, {
				"reserved", 312, 0 }, { "ipheaderpacketsection", 313, 0 }, {
				"ippayloadpacketsection", 314, 0 }, { "l2headerpacketsection", 315, 0 }, {
				"l2payloadpacketsection", 316, 0 }, { "mplslabelstacksection", 317, 0 }, {
				"mplspayloadpacketsection", 318, 0 }, { "meteringprocesssid", 319, 1 }, {
				"observationpointid", 320, 1 }, { "associationsid", 321, 1 },
		{ "reserved", 322, 0 }, { "selectortype", 323, 1 }, { "packetsobserved", 324, 1 }, {
				"packetsselected", 325, 1 }, { "accuracy", 326, 1 }, { "reserved", 327, 0 }, {
				"reserved", 328, 0 }, { "reserved", 329, 0 }, 
};

static const struct ipfix_identifier FRONTPAYLOAD = { "frontpayload", 32896, 0 };
static const struct ipfix_identifier BIFLOWTAB1[] = { { "revoctetdeltacount", 33793, 8 }, {
		"revpacketdeltacount", 33794, 8 }, { "reserved", 33795, 0 }, { "reserved", 33796, 0 }, {
		"reserved", 33797, 0 }, { "revtcpcontrolbits", 33798, 1 }, 
};
static const struct ipfix_identifier REVFRONTPAYLOAD = { "revfrontpayload", 33920, 0 };
static const struct ipfix_identifier BIFLOWTAB2[] = {

{ "revflowstartseconds", 33942, 4 }, { "revflowendseconds", 33943, 4 }, {
		"revflowstartmilliseconds", 33944, 8 }, { "revflowendmilliseconds", 33945, 8 }, 
};

#define PSAMP_STARTING_ID 300

static const struct ipfix_identifier PSAMPTAB[] = { { "selectorid", 300, 2 }, {
		"selectorinputsequencenumber", 301, 4 }, { "selectoralgorithm", 302, 1 }, { "reserved",
		303, 0 }, { "samplingpacketinterval", 304, 4 }, { "samplingpacketspace", 305, 4 }, {
		"samplingtimeinterval", 306, 4 }, { "samplingtimespace", 307, 4 }, { "samplingpopulation",
		308, 4 }, { "samplingsize", 309, 4 }, { "samplingprobabilityn", 310, 4 }, {
		"samplingprobabilitym", 311, 4 }, { "reserved", 312, 0 },
		{ "ipheaderpacketsection", 313, 0 }, { "ippayloadpacketsection", 314, 0 }, {
				"l2headerpacketsection", 315, 0 }, { "l2payloadpacketsection", 316, 0 }, {
				"mplslabelstacksection", 317, 0 }, { "mplspayloadpacketsection", 318, 0 }, {
				"meteringprocesssid", 319, 1 }, { "observationpointid", 320, 1 }, {
				"associationsid", 321, 1 }, { "reserved", 322, 0 }, { "selectortype", 323, 1 }, {
				"packetsobserved", 324, 1 }, { "packetsselected", 325, 1 }, { "accuracy", 326, 1 },
		{ "reserved", 327, 0 }, { "reserved", 328, 0 }, { "reserved", 329, 0 }, 
};

/* ANSI-C code produced by gperf version 3.0.3 */
/* Command-line: gperf -D -C -t -L ANSI-C gperf  */
/* Computed positions: -k'1-2,7,10,12-16,$' */

#if !((' ' == 32) && ('!' == 33) && ('"' == 34) && ('#' == 35) \
      && ('%' == 37) && ('&' == 38) && ('\'' == 39) && ('(' == 40) \
      && (')' == 41) && ('*' == 42) && ('+' == 43) && (',' == 44) \
      && ('-' == 45) && ('.' == 46) && ('/' == 47) && ('0' == 48) \
      && ('1' == 49) && ('2' == 50) && ('3' == 51) && ('4' == 52) \
      && ('5' == 53) && ('6' == 54) && ('7' == 55) && ('8' == 56) \
      && ('9' == 57) && (':' == 58) && (';' == 59) && ('<' == 60) \
      && ('=' == 61) && ('>' == 62) && ('?' == 63) && ('A' == 65) \
      && ('B' == 66) && ('C' == 67) && ('D' == 68) && ('E' == 69) \
      && ('F' == 70) && ('G' == 71) && ('H' == 72) && ('I' == 73) \
      && ('J' == 74) && ('K' == 75) && ('L' == 76) && ('M' == 77) \
      && ('N' == 78) && ('O' == 79) && ('P' == 80) && ('Q' == 81) \
      && ('R' == 82) && ('S' == 83) && ('T' == 84) && ('U' == 85) \
      && ('V' == 86) && ('W' == 87) && ('X' == 88) && ('Y' == 89) \
      && ('Z' == 90) && ('[' == 91) && ('\\' == 92) && (']' == 93) \
      && ('^' == 94) && ('_' == 95) && ('a' == 97) && ('b' == 98) \
      && ('c' == 99) && ('d' == 100) && ('e' == 101) && ('f' == 102) \
      && ('g' == 103) && ('h' == 104) && ('i' == 105) && ('j' == 106) \
      && ('k' == 107) && ('l' == 108) && ('m' == 109) && ('n' == 110) \
      && ('o' == 111) && ('p' == 112) && ('q' == 113) && ('r' == 114) \
      && ('s' == 115) && ('t' == 116) && ('u' == 117) && ('v' == 118) \
      && ('w' == 119) && ('x' == 120) && ('y' == 121) && ('z' == 122) \
      && ('{' == 123) && ('|' == 124) && ('}' == 125) && ('~' == 126))
/* The character set is not based on ISO-646.  */
#error "gperf generated tables don't work with this execution character set. Please report a bug to <bug-gnu-gperf@gnu.org>."
#endif

struct ipfix_midentifier
{
	char *name;
	uint16_t id;
};

#define TOTAL_KEYWORDS 254
#define MIN_WORD_LENGTH 6
#define MAX_WORD_LENGTH 27
#define MIN_HASH_VALUE 23
#define MAX_HASH_VALUE 751
/* maximum key range = 729, duplicates = 67 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int hash(register const char *str, register unsigned int len)
{
	static const unsigned short asso_values[] = { 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
			752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
			752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
			752, 752, 752, 752, 0, 752, 5, 25, 35, 20, 125, 15, 10, 0, 752, 752, 752, 752, 752,
			752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 0, 90, 20, 5, 5, 20, 210, 140, 5, 0, 160, 30, 35, 30, 0, 15, 5, 25, 5, 0,
					110, 120, 210, 60, 175, 0, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752,
					752, 752, 752, 752, 752, 752, 752, 752, 752, 752, 752 };
	register int hval = len;

	switch (hval) {
		default:
			hval += asso_values[(unsigned char)str[15]];
			/*FALLTHROUGH*/
		case 15:
			hval += asso_values[(unsigned char)str[14]];
			/*FALLTHROUGH*/
		case 14:
			hval += asso_values[(unsigned char)str[13]];
			/*FALLTHROUGH*/
		case 13:
			hval += asso_values[(unsigned char)str[12]];
			/*FALLTHROUGH*/
		case 12:
			hval += asso_values[(unsigned char)str[11]];
			/*FALLTHROUGH*/
		case 11:
		case 10:
			hval += asso_values[(unsigned char)str[9]];
			/*FALLTHROUGH*/
		case 9:
		case 8:
		case 7:
			hval += asso_values[(unsigned char)str[6]];
			/*FALLTHROUGH*/
		case 6:
		case 5:
		case 4:
		case 3:
		case 2:
			hval += asso_values[(unsigned char)str[1]];
			/*FALLTHROUGH*/
		case 1:
			hval += asso_values[(unsigned char)str[0]];
			break;
	}
	return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#ifdef __GNUC_STDC_INLINE__
__attribute__ ((__gnu_inline__))
#endif
#endif
const struct ipfix_midentifier *
in_word_set (register const char *str, register unsigned int len)
{
	static const struct ipfix_midentifier wordlist[] =
	{

		{	"sourceid", 149},

		{	"templateid", 145},

		{	"portid", 142},

		{	"selectorid", 300},

		{	"ismulticast", 206},

		{	"postvlanid", 59},

		{	"associationsid", 321},

		{	"postmcastoctettotalcount", 175},

		{	"tcpoptions", 209},

		{	"iptimetolive", 192},

		{	"reserved", 0},

		{	"reserved", 329},

		{	"reserved", 328},

		{	"reserved", 327},

		{	"reserved", 322},

		{	"reserved", 312},

		{	"reserved", 303},

		{	"reserved", 212},

		{	"reserved", 211},

		{	"reserved", 127},

		{	"reserved", 126},

		{	"reserved", 125},

		{	"reserved", 124},

		{	"reserved", 123},

		{	"reserved", 122},

		{	"reserved", 121},

		{	"reserved", 120},

		{	"reserved", 119},

		{	"reserved", 118},

		{	"reserved", 117},

		{	"reserved", 116},

		{	"reserved", 115},

		{	"reserved", 114},

		{	"reserved", 113},

		{	"reserved", 112},

		{	"reserved", 111},

		{	"reserved", 110},

		{	"reserved", 109},

		{	"reserved", 108},

		{	"reserved", 107},

		{	"reserved", 106},

		{	"reserved", 105},
		{	"reserved", 104},

		{	"reserved", 103},

		{	"reserved", 102},

		{	"reserved", 101},

		{	"reserved", 100},

		{	"reserved", 99},

		{	"reserved", 98},

		{	"reserved", 97},

		{	"reserved", 96},

		{	"reserved", 95},

		{	"reserved", 94},

		{	"reserved", 93},

		{	"reserved", 92},

		{	"reserved", 91},

		{	"reserved", 90},

		{	"reserved", 89},

		{	"reserved", 87},

		{	"reserved", 84},

		{	"reserved", 83},

		{	"reserved", 82},

		{	"reserved", 69},

		{	"reserved", 68},

		{	"reserved", 67},

		{	"reserved", 66},

		{	"reserved", 65},

		{	"reserved", 61},

		{	"reserved", 51},

		{	"reserved", 50},

		{	"reserved", 49},

		{	"reserved", 48},

		{	"reserved", 43},

		{	"reserved", 39},

		{	"reserved", 38},

		{	"reserved", 35},

		{	"reserved", 34},

		{	"reserved", 3},

		{	"notsentoctettotalcount", 168},

		{	"tcpwindowsize", 186},

		{	"postmcastoctetdeltacount", 20},

		{	"droppedoctettotalcount", 134},

		{	"flowid", 148},

		{	"samplingsize", 309},

		{	"ipversion", 60},

		{	"ipv4options", 208},

		{	"frontpayload", 32896},

		{	"ipprecedence", 196},

		{	"notsentpackettotalcount", 167},

		{	"tcpcontrolbits", 6},

		{	"linecardid", 141},

		{	"notsentflowtotalcount", 166},

		{	"droppedpackettotalcount", 135},

		{	"postoctettotalcount", 171},

		{	"tcpdestinationport", 183},

		{	"postoctetdeltacount", 23},

		{	"destinationtransportport", 11},

		{	"destinationmacaddress", 80},

		{	"droppedoctetdeltacount", 132},

		{	"tcpsourceport", 182},

		{	"packetsselected", 325},

		{	"postpackettotalcount", 172},

		{	"sourcemacaddress", 56},

		{	"samplingtimespace", 307},

		{	"sourcetransportport", 7},

		{	"postpacketdeltacount", 24},

		{	"sourceipv4address", 8},

		{	"revfrontpayload", 33920},

		{	"exportedoctettotalcount", 40},

		{	"droppedpacketdeltacount", 133},

		{	"flowinactivetimeout", 37},

		{	"icmpcodeipv4", 177},

		{	"flowendreason", 136},

		{	"flowendmicroseconds", 155},

		{	"ingressinterface", 10},

		{	"flowendseconds", 151},

		{	"samplingtimeinterval", 306},

		{	"flowstartseconds", 150},

		{	"icmptypeipv4", 176},

		{	"identificationipv4", 54},

		{	"maximumttl", 53},

		{	"flowendsysuptime", 21},

		{	"exportingprocessid", 144},

		{	"flowendmilliseconds", 153},

		{	"minimumttl", 52},

		{	"tcpurgentpointer", 187},

		{	"protocolidentifier", 4},

		{	"postdestinationmacaddress", 57},

		{	"flowstartdeltamicroseconds", 158},

		{	"meteringprocessid", 143},

		{	"meteringprocesssid", 319},

		{	"vlanid", 58},

		{	"observationpointid", 320},

		{	"flowstartnanoseconds", 156},

		{	"flowstartmicroseconds", 154},

		{	"flowendnanoseconds", 157},

		{	"internetheaderlengthipv4", 207},

		{	"fragmentoffsetipv4", 88},

		{	"exporteripv4address", 130},

		{	"octetdeltacount", 1},

		{	"udpdestinationport", 181},

		{	"postsourcemacaddress", 81},

		{	"flowstartmilliseconds", 152},

		{	"revoctetdeltacount", 33793},

		{	"ipdiffservcodepoint", 195},

		{	"packettotalcount", 86},

		{	"udpsourceport", 180},

		{	"packetdeltacount", 2},

		{	"revtcpcontrolbits", 33798},

		{	"classofserviceipv4", 5},

		{	"flowenddeltamicroseconds", 159},

		{	"packetsobserved", 324},

		{	"sourceipv6address", 27},

		{	"octettotalcount", 85},

		{	"selectortype", 323},

		{	"destinationipv4address", 12},

		{	"flowstartsysuptime", 22},

		{	"selectorinputsequencenumber", 301},

		{	"octetdeltasumofsquares", 198},

		{	"accuracy", 326},

		{	"flowdurationmicroseconds", 162},

		{	"ipv6extensionheaders", 64},

		{	"ipclassofservice", 194},

		{	"flowactivetimeout", 36},

		{	"samplingpopulation", 308},

		{	"totallengthipv4", 190},

		{	"postclassofserviceipv4", 55},

		{	"sourceipv4prefix", 44},

		{	"flowdurationmilliseconds", 161},

		{	"postmcastpackettotalcount", 174},

		{	"ignoredoctettotalcount", 165},

		{	"igmptype", 33},

		{	"samplingpacketspace", 305},

		{	"postmcastpacketdeltacount", 19},

		{	"octettotalsumofsquares", 199},

		{	"paddingoctets", 210},

		{	"ipheaderpacketsection", 313},

		{	"wlanssid", 147},

		{	"mplstoplabelttl", 200},

		{	"selectoralgorithm", 302},

		{	"revpacketdeltacount", 33794},

		{	"l2headerpacketsection", 315},

		{	"ignoredpackettotalcount", 164},

		{	"exporteripv6address", 131},

		{	"icmptypecodeipv4", 32},

		{	"wlanchannelid", 146},

		{	"ippayloadpacketsection", 314},

		{	"destinationipv4prefix", 45},

		{	"samplingpacketinterval", 304},

		{	"classofserviceipv6", 137},

		{	"egressinterface", 14},

		{	"l2payloadpacketsection", 316},

		{	"revflowstartseconds", 33942},

		{	"mplstoplabelexp", 203},

		{	"systeminittimemilliseconds", 160},

		{	"destinationipv6address", 28},

		{	"icmpcodeipv6", 179},

		{	"flowkeyindicator", 173},

		{	"icmptypeipv6", 178},

		{	"maximumpacketlength", 26},

		{	"exportedmessagetotalcount", 41},

		{	"postclassofserviceipv6", 138},

		{	"minimumpacketlength", 25},

		{	"samplingprobabilityn", 310},

		{	"sourceipv6prefix", 170},

		{	"revflowendseconds", 33943},

		{	"samplingprobabilitym", 311},

		{	"exportedflowtotalcount", 42},

		{	"revflowendmilliseconds", 33945},

		{	"ipnexthopipv4address", 15},

		{	"observedflowtotalcount", 163},

		{	"revflowstartmilliseconds", 33944},

		{	"destinationipv6prefix", 169},

		{	"mplslabelstackentry9", 78},

		{	"mplslabelstackentry10", 79},

		{	"sourceipv4mask", 9},

		{	"mplslabelstackentry2", 71},

		{	"mplslabelstacksection", 317},

		{	"bgpdestinationasnumber", 17},

		{	"mplstoplabelipv4address", 47},

		{	"tcpacknowledgementnumber", 185},

		{	"mplslabelstackentry8", 77},

		{	"mplslabelstackentry7", 76},

		{	"destinationipv4mask", 13},

		{	"mplslabelstackentry5", 74},

		{	"mplstoplabeltype", 46},

		{	"bgpnextadjacentasnumber", 128},

		{	"mplslabelstackentry3", 72},

		{	"mplstoplabelstackentry", 70},

		{	"mplslabelstackentry4", 73},

		{	"ipnexthopipv6address", 62},

		{	"icmptypecodeipv6", 139},

		{	"nextheaderipv6", 193},

		{	"tcpsequencenumber", 184},

		{	"payloadlengthipv6", 191},

		{	"sourceipv6mask", 29},

		{	"mplstoplabelipv6address", 140},

		{	"mplspayloadpacketsection", 318},

		{	"destinationipv6mask", 30},

		{	"mplslabelstackdepth", 202},

		{	"mplslabelstackentry6", 75},

		{	"fragmentflagsipv4", 197},

		{	"bgpnexthopipv4address", 18},

		{	"mplslabelstacklength", 201},

		{	"bgpprevadjacentasnumber", 129},

		{	"flowlabelipv6", 31},

		{	"ipheaderlength", 189},

		{	"ippayloadlength", 204},

		{	"tcpheaderlength", 188},

		{	"bgpnexthopipv6address", 63},

		{	"bgpsourceasnumber", 16},

		{	"mplspayloadlength", 214},

		{	"udpmessagelength", 205},

		{	"headerlengthipv4", 213}
	};

	static const short lookup[] =
	{
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 0,
		-1, 1, 2, -1, -1, -1, 3, 4,
		-1, -1, -1, 5, -1, -1, -1, 6,
		-1, -1, -1, -1, 7, 8, -1, 9,
		-304, -244, -68, -1, 78, 79, 80, -1,
		-1, 81, -1, -1, -1, 82, 83, -1,
		84, -1, 85, 86, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 87, 88, 89,
		90, 91, -1, 92, 93, -1, -1, -1,
		94, 95, -1, -1, -1, -1, 96, -1,
		97, 98, 99, -1, 100, -1, -1, -1,
		-1, 101, 102, 103, -1, 104, 105, -1,
		106, -1, -1, 107, -1, -1, 108, -1,
		-1, -1, -1, 109, 110, -1, -1, 111,
		112, 113, -1, 114, -1, -1, 115, 116,
		117, 118, 119, -1, 120, 121, -1, 122,
		123, 124, 125, -1, 126, -1, 127, -1,
		-1, -1, -1, -1, 128, 129, 130, -1,
		-1, 131, -1, 132, -1, 133, 134, -1,
		135, -1, -1, -1, -1, -1, 136, -1,
		-1, -1, 137, 138, 139, -1, -1, 140,
		-1, 141, 142, -1, 143, 144, -1, 145,
		-1, 146, -1, -1, 147, 148, 149, 150,
		151, -1, 152, -1, -1, 153, -1, 154,
		-1, -1, -1, -1, 155, 156, -1, -1,
		-1, 157, -1, -1, -1, -1, 158, 159,
		160, 161, 162, 163, 164, -1, 165, -1,
		166, -1, -1, -1, 167, -1, -1, 168,
		169, -1, 170, 171, 172, 173, -1, 174,
		175, -1, -1, 176, -1, -1, -1, -1,
		-1, -1, 177, -1, 178, -1, 179, -1,
		180, -1, 181, -1, 182, 183, -1, 184,
		-1, 185, -1, -1, -1, 186, -1, -1,
		-1, 187, -1, -1, -1, -1, -1, 188,
		189, -1, 190, -1, 191, -1, 192, 193,
		-1, -1, -1, -1, -1, 194, 195, -1,
		-1, -1, -1, 196, -1, -1, -1, -1,
		-1, -1, -1, -1, 197, 198, -1, 199,
		200, -1, 201, -1, 202, 203, 204, 205,
		-1, -1, 206, -1, 207, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, 208, -1, -1, -1, -1,
		-1, -1, -1, 209, -1, 210, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, 211, -1, 212, -1, -1, -1, -1,
		-1, -1, -1, -1, 213, 214, -1, -1,
		215, 216, 217, 218, 219, 220, 221, -1,
		-1, -1, -1, 222, -1, -1, -1, 223,
		224, 225, -1, 226, -1, 227, -1, 228,
		-1, -1, -1, -1, -1, -1, -1, 229,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 230, -1, -1,
		-1, -1, -1, 231, -1, -1, -1, -1,
		-1, -1, -1, 232, -1, -1, 233, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		234, -1, 235, -1, -1, -1, 236, 237,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, 238, -1, -1, -1, -1, 239, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, 240, -1, -1, -1, -1, -1, -1,
		241, -1, -1, -1, 242, -1, -1, -1,
		243, -1, -1, 244, -1, -1, -1, -1,
		245, -1, -1, -1, -1, -1, 246, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, 247, -1, -1, -1, -1, 248, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, 249, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, 250, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, 251, -1, -1, -1, -1, -1, -1,
		-1, -1, 252, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, -1,
		-1, -1, -1, -1, -1, -1, -1, 253
	};

	if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
	{
		register int key = hash (str, len);

		if (key <= MAX_HASH_VALUE && key >= 0)
		{
			register int index = lookup[key];

			if (index >= 0)
			{
				register const char *s = wordlist[index].name;

				if (*str == *s && !strcmp (str + 1, s + 1))
				return &wordlist[index];
			}
			else if (index < -TOTAL_KEYWORDS)
			{
				register int offset = - 1 - TOTAL_KEYWORDS - index;
				register const struct ipfix_midentifier *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
				register const struct ipfix_midentifier *wordendptr = wordptr + -lookup[offset + 1];

				while (wordptr < wordendptr)
				{
					register const char *s = wordptr->name;

					if (*str == *s && !strcmp (str + 1, s + 1))
					return wordptr;
					wordptr++;
				}
			}
		}
	}
	return 0;
}

/* End of gperf-generated code */

static inline int ipfix_rangecheck_ipfix(int n)
{
	return (n >= 0 && n < sizeof(IPFIXTAB)/sizeof(*IPFIXTAB));
}

static inline int ipfix_rangecheck_psamp(n)
{
	return (n >= PSAMP_STARTING_ID && n < PSAMP_STARTING_ID + (sizeof(PSAMPTAB)/sizeof(*PSAMPTAB)));
}

/* rangecheck the id for ipfix and psamp */
int ipfix_id_rangecheck(int id)
{
	return (ipfix_rangecheck_ipfix(id) || ipfix_rangecheck_psamp(id));
}

/* lookup a certain ipfix ID into its name */
const struct ipfix_identifier * ipfix_id_lookup(int n)
{
	/* ID is IPFIX one */
	if (ipfix_rangecheck_ipfix(n)) {
		return &IPFIXTAB[n];
	}

	/* ID is PSAMP one */
	if (ipfix_rangecheck_psamp(n)) {
		/* have to normalize */
		n -= PSAMP_STARTING_ID;
		return &PSAMPTAB[n];
	}

	if (n==32896) {
		printf("frontpayload!\n");
		return &FRONTPAYLOAD;
	}

	// stuff for biflow types
	if (n>=33793 && n<=33798) {
		return &BIFLOWTAB1[n-33793];
	}
	if (n==33920)
		return &REVFRONTPAYLOAD;
	if (n>=33942 && n<=33945) {
		return &BIFLOWTAB2[n-33942];
	}

	return NULL;
}

/*
 lookup an ipfix name into its ID
 int because we need -1 for "not found"
 */
int ipfix_name_lookup(const char *name)
{
	const struct ipfix_midentifier *tmp;

	if (!(tmp=in_word_set(name, strlen(name)))) {
		/* not found */
		return -1;
	}

	return (int)tmp->id;
}

#ifdef __cplusplus
}
#endif
