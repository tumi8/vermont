#include <string.h>
#include "ipfix_names.h"

#ifdef __cplusplus
extern "C" {
#endif

static const struct ipfix_identifier IPFIXTAB[] = {
	{ "reserved", 0, 0 },
	{ "inoctetdeltacount", 1, 8 },
	{ "inpacketdeltacount", 2, 8 },
	{ "reserved", 3, 0 },
	{ "protocolidentifier", 4, 1 },
	{ "classofserviceipv4", 5, 1 },
	{ "tcpcontrolbits", 6, 1 },
	{ "sourcetransportport", 7, 2 },
	{ "sourceipv4address", 8, 4 },
	{ "sourceipv4mask", 9, 1 },
	{ "ingressinterface", 10, 4 },
	{ "destinationtransportport", 11, 2 },
	{ "destinationipv4address", 12, 4 },
	{ "destinationipv4mask", 13, 1 },
	{ "egressinterface", 14, 4 },
	{ "ipnexthopipv4address", 15, 4 },
	{ "bgpsourceasnumber", 16, 2 },
	{ "bgpdestinationasnumber", 17, 2 },
	{ "bgpnexthopipv4address", 18, 4 },
	{ "outmulticastpacketcount", 19, 8 },
	{ "outmulticastoctetcount", 20, 8 },
	{ "flowendtime", 21, 4 },
	{ "flowcreationtime", 22, 4 },
	{ "outoctetdeltacount", 23, 8 },
	{ "outpacketdeltacount", 24, 8 },
	{ "minimumpacketlength", 25, 2 },
	{ "maximumpacketlength", 26, 2 },
	{ "sourceipv6address", 27, 8 },
	{ "destinationipv6address", 28, 8 },
	{ "sourceipv6mask", 29, 1 },
	{ "destinationipv6mask", 30, 0 },
	{ "flowlabelipv6", 31, 0 },
	{ "icmptypecode", 32, 2 },
	{ "igmptype", 33, 1 },
	{ "reserved", 34, 0 },
	{ "reserved", 35, 0 },
	{ "flowactivetimeout", 36, 2 },
	{ "flowinactivetimeout", 37, 2 },
	{ "reserved", 38, 0 },
	{ "reserved", 39, 0 },
	{ "exportedoctetcount", 40, 8 },
	{ "exportedpacketcount", 41, 8 },
	{ "exportedflowcount", 42, 8 },
	{ "reserved", 43, 0 },
	{ "sourceipv4prefix", 44, 4 },
	{ "destinationipv4prefix", 45, 0 },
	{ "mplstoplabeltype", 46, 1 },
	{ "mplstoplabelipv4address", 47, 4 },
	{ "reserved", 48, 0 },
	{ "reserved", 49, 0 },
	{ "reserved", 50, 0 },
	{ "reserved", 51, 0 },
	{ "minimumttl", 52, 1 },
	{ "maximumttl", 53, 1 },
	{ "identificationipv4", 54, 1 },
	{ "reserved", 55, 0 },
	{ "sourcemacaddress", 56, 1 },
	{ "reserved", 57, 0 },
	{ "reserved", 58, 0 },
	{ "reserved", 59, 0 },
	{ "ipversion", 60, 0 },
	{ "reserved", 61, 0 },
	{ "ipnexthopipv6address", 62, 8 },
	{ "bgpnexthopipv6address", 63, 8 },
	{ "ipv6optionheaders", 64, 4 },
	{ "reserved", 65, 0 },
	{ "reserved", 66, 0 },
	{ "reserved", 67, 0 },
	{ "reserved", 68, 0 },
	{ "reserved", 69, 0 },
	{ "mplslabelstackentry1", 70, 4 },
	{ "mplslabelstackentry2", 71, 4 },
	{ "mplslabelstackentry3", 72, 4 },
	{ "mplslabelstackentry4", 73, 4 },
	{ "mplslabelstackentry5", 74, 4 },
	{ "mplslabelstackentry6", 75, 4 },
	{ "mplslabelstackentry7", 76, 4 },
	{ "mplslabelstackentry8", 77, 4 },
	{ "mplslabelstackentry9", 78, 4 },
	{ "mplslabelstackentry10", 79, 4 },
	{ "reserved", 80, 0 },
	{ "reserved", 81, 0 },
	{ "reserved", 82, 0 },
	{ "reserved", 83, 0 },
	{ "reserved", 84, 0 },
	{ "octettotalcount", 85, 8 },
	{ "packettotalcount", 86, 8 },
	{ "reserved", 87, 0 },
	{ "reserved", 88, 0 },
	{ "reserved", 89, 0 },
	{ "reserved", 90, 0 },
	{ "reserved", 91, 0 },
	{ "reserved", 92, 0 },
	{ "reserved", 93, 0 },
	{ "reserved", 94, 0 },
	{ "reserved", 95, 0 },
	{ "reserved", 96, 0 },
	{ "reserved", 97, 0 },
	{ "reserved", 98, 0 },
	{ "reserved", 99, 0 },
	{ "reserved", 100, 0 },
	{ "reserved", 101, 0 },
	{ "reserved", 102, 0 },
	{ "reserved", 103, 0 },
	{ "reserved", 104, 0 },
	{ "reserved", 105, 0 },
	{ "reserved", 106, 0 },
	{ "reserved", 107, 0 },
	{ "reserved", 108, 0 },
	{ "reserved", 109, 0 },
	{ "reserved", 110, 0 },
	{ "reserved", 111, 0 },
	{ "reserved", 112, 0 },
	{ "reserved", 113, 0 },
	{ "reserved", 114, 0 },
	{ "reserved", 115, 0 },
	{ "reserved", 116, 0 },
	{ "reserved", 117, 0 },
	{ "reserved", 118, 0 },
	{ "reserved", 119, 0 },
	{ "reserved", 120, 0 },
	{ "reserved", 121, 0 },
	{ "reserved", 122, 0 },
	{ "reserved", 123, 0 },
	{ "reserved", 124, 0 },
	{ "reserved", 125, 0 },
	{ "reserved", 126, 0 },
	{ "reserved", 127, 0 },
	{ "bgpnexthopasnumber", 128, 2 },
	{ "ipnexthopasnumber", 129, 2 },
	{ "exporteripv4address", 130, 4 },
	{ "exporteripv6address", 131, 8 },
	{ "droppedoctetdeltacount", 132, 8 },
	{ "droppedpacketdeltacount", 133, 8 },
	{ "droppedoctettotalcount", 134, 8 },
	{ "droppedpackettotalcount", 135, 8 },
	{ "flowendreason", 136, 1 },
	{ "classofserviceipv6", 137, 1 },
	{ "octetdeltacount", 138, 8 },
	{ "packetdeltacount", 139, 8 },
	{ "mplstoplabelipv6address", 140, 8 },
	{ "linecardid", 141, 4 },
	{ "portid", 142, 4 },
	{ "meteringprocessid", 143, 4 },
	{ "exportingprocessid", 144, 4 },
	{ "templateid", 145, 2 },
	{ "wlanchannelid", 146, 1 },
	{ "wlanssid", 147, 0 },
	{ "flowid", 148, 4 },
	{ "sourceid", 149, 4 },
	{ "flowstartseconds", 150, 4 },
	{ "flowendseconds", 151, 4 },
	{ "flowstartmilliseconds", 152, 4 },
	{ "flowendmilliseconds", 153, 4 },
	{ "flowstartmicroseconds", 154, 4 },
	{ "flowendmicroseconds", 155, 4 },
	{ "flowstartnanoseconds", 156, 4 },
	{ "flowendnanoseconds", 157, 4 },
	{ "flowstartdeltamicroseconds", 158, 4 },
	{ "flowenddeltamicroseconds", 159, 4 },
	{ "systeminittimemilliseconds", 160, 4 },
	{ "flowdurationmilliseconds", 161, 4 },
	{ "flowdurationmicroseconds", 162, 4 },
	{ "observedflowtotalcount", 163, 8 },
	{ "ignoredpackettotalcount", 164, 8 },
	{ "ignoredoctettotalcount", 165, 8 },
	{ "notsentflowtotalcount", 166, 8 },
	{ "notsentpackettotalcount", 167, 8 },
	{ "notsentoctettotalcount", 168, 8 },
	{ "destinationipv6prefix", 169, 1 },
	{ "sourceipv6prefix", 170, 1 },
	{ "postoctettotalcount", 171, 8 },
	{ "postpackettotalcount", 172, 8 },
	{ "flowkeyindicator", 173, 8 },
	{ "postmcastpackettotalcount", 174, 8 },
	{ "postmcastoctettotalcount", 175, 8 },
	{ "icmptypeipv4", 176, 1 },
	{ "icmpcodeipv4", 177, 1 },
	{ "icmptypeipv6", 178, 2 },
	{ "icmpcodeipv6", 179, 2 },
	{ "udpsourceport", 180, 0 },
	{ "udpdestinationport", 181, 2 },
	{ "tcpsourceport", 182, 2 },
	{ "tcpdestinationport", 183, 2 },
	{ "tcpsequencenumber", 184, 4 },
	{ "tcpacknowledgementnumber", 185, 4 },
	{ "tcpwindowsize", 186, 2 },
	{ "tcpurgentpointer", 187, 2 },
	{ "tcpheaderlength", 188, 2 },
	{ "ipheaderlength", 189, 2 },
	{ "packetlengthipv4", 190, 2 },
	{ "payloadlengthipv6", 191, 4 },
	{ "iptimetolive", 192, 1 },
	{ "nextheaderipv6", 193, 1 },
	{ "ipclassofservice", 194, 1 },
	{ "ipdiffservcodepoint", 195, 1 },
	{ "ipprecedence", 196, 1 },
	{ "fragmentflagsipv4", 197, 1 },
	{ "octetdeltasumofsquares", 198, 8 },
	{ "octettotalsumofsquares", 199, 8 },
	{ "mplstoplabelttl", 200, 4 },
	{ "mplslabelstacksize", 201, 4 },
	{ "mplslabelstackdepth", 202, 4 },
	{ "mplstoplabelexp", 203, 1 },
	{ "reserved", 204, 0 },
	{ "reserved", 205, 0 },
	{ "reserved", 206, 0 },
	{ "headerlengthipv4", 207, 1 },
	{ "ipv4options", 208, 8 },
	{ "tcpoptions", 209, 8 },
	{ "paddingoctets", 210, 0 },
};

static const struct ipfix_identifier PSAMPTAB[] = {
	{ "optiontemplateid", 1024, 2 },
	{ "sequencenumber", 1025, 4 },

	/* FIXME: OCTET STRING */
	{ "packetsample", 1026, 0 },

	{ "samplingalgorithm", 1027, 1 },
	{ "filteringalgorithm", 1028, 1 },
	{ "samplingpacketinterval", 1029, 4 },
	{ "samplingpacketspace", 1030, 4 },

	/* dateTimeMicroSeconds = 4 ? */
	{ "samplingtimeinterval", 1031, 4 },
	{ "samplingtimespace", 1032, 4 },

	{ "samplingpopulation", 1033, 4 },
	{ "samplingsize", 1034, 4 },
	{ "samplingprobability", 1035, 4 },
	{ "headertype", 1036, 1 },
	{ "hashfunction", 1037, 1 }
};

/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -D -C -t -L ANSI-C  */
struct ipfix_midentifier { char *name; uint16_t id; };

#define TOTAL_KEYWORDS 225
#define MIN_WORD_LENGTH 6
#define MAX_WORD_LENGTH 26
#define MIN_HASH_VALUE 12
#define MAX_HASH_VALUE 238
/* maximum key range = 227, duplicates = 102 */

static unsigned int hash (register const char *str, register unsigned int len)
{
	static const unsigned char asso_values[] =
	{
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239,  15,  45,
		10,  20,   2,  25,  92,  30,  35,  40, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239,  25,   2,
		0,  95, 127, 239, 114,  15, 239,  15,  39,  85,
		80, 100,  20, 239,   4,  40,  55,  30, 239,   5,
		120,   0, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239, 239, 239, 239, 239,
		239, 239, 239, 239, 239, 239
	};
	return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}


const struct ipfix_midentifier * in_word_set (register const char *str, register unsigned int len)
{
	static const struct ipfix_midentifier wordlist[] =
	{
		{"reserved", 0},
		{"reserved", 206},
		{"reserved", 205},
		{"reserved", 204},
		{"reserved", 127},
		{"reserved", 126},
		{"reserved", 125},
		{"reserved", 124},
		{"reserved", 123},
		{"reserved", 122},
		{"reserved", 121},
		{"reserved", 120},
		{"reserved", 119},
		{"reserved", 118},
		{"reserved", 117},
		{"reserved", 116},
		{"reserved", 115},
		{"reserved", 114},
		{"reserved", 113},
		{"reserved", 112},
		{"reserved", 111},
		{"reserved", 110},
		{"reserved", 109},
		{"reserved", 108},
		{"reserved", 107},
		{"reserved", 106},
		{"reserved", 105},
		{"reserved", 104},
		{"reserved", 103},
		{"reserved", 102},
		{"reserved", 101},
		{"reserved", 100},
		{"reserved", 99},
		{"reserved", 98},
		{"reserved", 97},
		{"reserved", 96},
		{"reserved", 95},
		{"reserved", 94},
		{"reserved", 93},
		{"reserved", 92},
		{"reserved", 91},
		{"reserved", 90},
		{"reserved", 89},
		{"reserved", 88},
		{"reserved", 87},
		{"reserved", 84},
		{"reserved", 83},
		{"reserved", 82},
		{"reserved", 81},
		{"reserved", 80},
		{"reserved", 69},
		{"reserved", 68},
		{"reserved", 67},
		{"reserved", 66},
		{"reserved", 65},
		{"reserved", 61},
		{"reserved", 59},
		{"reserved", 58},
		{"reserved", 57},
		{"reserved", 55},
		{"reserved", 51},
		{"reserved", 50},
		{"reserved", 49},
		{"reserved", 48},
		{"reserved", 43},
		{"reserved", 39},
		{"reserved", 38},
		{"reserved", 35},
		{"reserved", 34},
		{"reserved", 3},
		{"wlanssid", 147},
		{"wlanchannelid", 146},
		{"classofserviceipv4", 5},
		{"portid", 142},
		{"icmptypeipv4", 176},
		{"icmpcodeipv4", 177},
		{"destinationipv4mask", 13},
		{"destinationipv6mask", 30},
		{"identificationipv4", 54},
		{"ipnexthopasnumber", 129},
		{"packetlengthipv4", 190},
		{"protocolidentifier", 4},
		{"bgpsourceasnumber", 16},
		{"bgpnexthopasnumber", 128},
		{"sourceid", 149},
		{"linecardid", 141},
		{"bgpdestinationasnumber", 17},
		{"sequencenumber", 1025},
		{"samplingprobability", 1035},
		{"destinationipv4address", 12},
		{"destinationipv6address", 28},
		{"templateid", 145},
		{"ipv4options", 208},
		{"sourceipv4mask", 9},
		{"sourceipv6mask", 29},
		{"ipv6optionheaders", 64},
		{"paddingoctets", 210},
		{"tcpurgentpointer", 187},
		{"ipnexthopipv4address", 15},
		{"ipnexthopipv6address", 62},
		{"tcpsequencenumber", 184},
		{"droppedoctetdeltacount", 132},
		{"droppedoctettotalcount", 134},
		{"droppedpacketdeltacount", 133},
		{"droppedpackettotalcount", 135},
		{"destinationtransportport", 11},
		{"tcpacknowledgementnumber", 185},
		{"bgpnexthopipv4address", 18},
		{"bgpnexthopipv6address", 63},
		{"inoctetdeltacount", 1},
		{"inpacketdeltacount", 2},
		{"ipdiffservcodepoint", 195},
		{"packettotalcount", 86},
		{"packetdeltacount", 139},
		{"ignoredoctettotalcount", 165},
		{"ignoredpackettotalcount", 164},
		{"postoctettotalcount", 171},
		{"postpackettotalcount", 172},
		{"sourcemacaddress", 56},
		{"sourceipv4address", 8},
		{"sourceipv6address", 27},
		{"udpsourceport", 180},
		{"postmcastoctettotalcount", 175},
		{"samplingtimeinterval", 1031},
		{"postmcastpackettotalcount", 174},
		{"samplingpacketinterval", 1029},
		{"meteringprocessid", 143},
		{"udpdestinationport", 181},
		{"ipversion", 60},
		{"tcpoptions", 209},
		{"systeminittimemilliseconds", 160},
		{"mplslabelstackentry4", 73},
		{"tcpcontrolbits", 6},
		{"classofserviceipv6", 137},
		{"exportingprocessid", 144},
		{"sourcetransportport", 7},
		{"mplslabelstackentry2", 71},
		{"optiontemplateid", 1024},
		{"igmptype", 33},
		{"icmptypeipv6", 178},
		{"icmpcodeipv6", 179},
		{"mplstoplabelexp", 203},
		{"mplslabelstackentry10", 79},
		{"icmptypecode", 32},
		{"ipprecedence", 196},
		{"iptimetolive", 192},
		{"tcpsourceport", 182},
		{"mplslabelstackentry3", 72},
		{"ingressinterface", 10},
		{"ipclassofservice", 194},
		{"packetsample", 1026},
		{"tcpdestinationport", 183},
		{"payloadlengthipv6", 191},
		{"mplslabelstackentry5", 74},
		{"headerlengthipv4", 207},
		{"flowid", 148},
		{"minimumttl", 52},
		{"maximumttl", 53},
		{"mplslabelstackentry7", 76},
		{"samplingpopulation", 1033},
		{"mplstoplabelttl", 200},
		{"mplslabelstackentry8", 77},
		{"destinationipv4prefix", 45},
		{"destinationipv6prefix", 169},
		{"samplingalgorithm", 1027},
		{"ipheaderlength", 189},
		{"mplslabelstackentry9", 78},
		{"fragmentflagsipv4", 197},
		{"samplingsize", 1034},
		{"flowkeyindicator", 173},
		{"mplstoplabelipv4address", 47},
		{"mplstoplabelipv6address", 140},
		{"mplslabelstackentry1", 70},
		{"samplingtimespace", 1032},
		{"exporteripv4address", 130},
		{"samplingpacketspace", 1030},
		{"exporteripv6address", 131},
		{"notsentflowtotalcount", 166},
		{"notsentoctettotalcount", 168},
		{"notsentpackettotalcount", 167},
		{"octetdeltasumofsquares", 198},
		{"octettotalsumofsquares", 199},
		{"tcpwindowsize", 186},
		{"exportedflowcount", 42},
		{"exportedoctetcount", 40},
		{"exportedpacketcount", 41},
		{"octettotalcount", 85},
		{"octetdeltacount", 138},
		{"outoctetdeltacount", 23},
		{"outpacketdeltacount", 24},
		{"sourceipv4prefix", 44},
		{"sourceipv6prefix", 170},
		{"outmulticastoctetcount", 20},
		{"observedflowtotalcount", 163},
		{"outmulticastpacketcount", 19},
		{"flowendseconds", 151},
		{"flowstartseconds", 150},
		{"tcpheaderlength", 188},
		{"flowendnanoseconds", 157},
		{"flowendmilliseconds", 153},
		{"flowendmicroseconds", 155},
		{"nextheaderipv6", 193},
		{"flowstartnanoseconds", 156},
		{"flowstartmilliseconds", 152},
		{"flowstartmicroseconds", 154},
		{"flowenddeltamicroseconds", 159},
		{"flowdurationmicroseconds", 162},
		{"flowdurationmilliseconds", 161},
		{"flowstartdeltamicroseconds", 158},
		{"mplstoplabeltype", 46},
		{"mplslabelstackentry6", 75},
		{"mplslabelstacksize", 201},
		{"flowactivetimeout", 36},
		{"flowinactivetimeout", 37},
		{"egressinterface", 14},
		{"hashfunction", 1037},
		{"minimumpacketlength", 25},
		{"mplslabelstackdepth", 202},
		{"maximumpacketlength", 26},
		{"headertype", 1036},
		{"flowendreason", 136},
		{"filteringalgorithm", 1028},
		{"flowlabelipv6", 31},
		{"flowendtime", 21},
		{"flowcreationtime", 22}
	};

	static const short lookup[] =
	{
		-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1, -240,   70, -225,  -70,
		-1,   -1,   71,   -1,   -1,   -1,   72,   -1,
		-1,   -1,   73,   -1,   -1, -256, -151,   -2,
		-1,   -1, -265,   78,   79,   -1,   80, -149,
		-2,   -1,   81,   -1,   -1,   -1,   82,   83,
		84,   85,   -1,   86,   -1,   -1,   -1,   -1,
		-1,   -1,   87,   88,   -1,   -1, -289, -136,
		-2,   91,   92, -132,   -2, -293, -128,   -3,
		95,   96,   -1, -296,  100, -310, -306,  105,
		-122,   -2,   -1,  106, -124,   -2, -462,  109,
		110,  111,   -1, -460,  114,  115,  116,  117,
		118, -453,  121, -336,  124,  125,  126,  127,
		128,  129,  130,  131,   -1,  132, -103,   -2,
		133,  134,  135,  136,  137,   -1,  138, -451,
		141,  142, -449,  146,   -1,  147, -441,  150,
		151,  152,  153,   -1,  154,  155, -362,  158,
		-69,   -2,  159,  160,  161, -439,  164,  165,
		-1,  166,  167, -437, -435,   -1,  172,   -1,
		173,   -1, -385,   -1,  177,  178,  179,  -51,
		-3,   -1, -390,  182,  -45,   -2,   -1,  183,
		184,  185, -397,  -39,   -2,  188,  189,   -1,
		-433, -405,  194,  -33,   -2,  195,   -1,  196,
		197,  198, -428,  202, -415,  -22,   -2, -420,
		-1,  208,  -20,   -3,  209,  210,  211,  212,
		-1,  213,  -26,   -3,   -1,  214,  215,  -35,
		-2,  -55,   -2,  -57,   -2,  -63,   -2,  -77,
		-2,   -1, -447,  219,  220,   -9,   -3,  -82,
		-3,  -86,   -2, -106,   -2,   -1,  221,   -1,
		222,  223, -113,   -2, -118,   -2,  224
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


static inline int ipfix_rangecheck_ipfix(int n)
{
        return (n >= 0 && n < sizeof(IPFIXTAB)/sizeof(*IPFIXTAB));
}

static inline int ipfix_rangecheck_psamp(n)
{
        return (n >= 1024 && n < 1024 + (sizeof(PSAMPTAB)/sizeof(*PSAMPTAB)));
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
        //if(n >= 0 && n < sizeof(IPFIXTAB)/sizeof(*IPFIXTAB)) {
        if(ipfix_rangecheck_ipfix(n)) {
		return &IPFIXTAB[n];
	}

	/* ID is PSAMP one */
        //if(n >= 1024 && n < 1024 + (sizeof(PSAMPTAB)/sizeof(*PSAMPTAB))) {
        if(ipfix_rangecheck_psamp(n)) {
		/* have to normalize */
		n -= 1024;
		return &PSAMPTAB[n];
	}

	return NULL;
}


/*
 lookup an ipfix name into its ID
 int because we need -1 for "not found"
 */
int ipfix_name_lookup(char *name)
{
	const struct ipfix_midentifier *tmp;

	if(!(tmp=in_word_set(name, strlen(name)))) {
		/* not found */
		return -1;
	}

	return (int)tmp->id;
}

#ifdef __cplusplus
}
#endif
