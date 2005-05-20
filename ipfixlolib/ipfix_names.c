#include <string.h>
#include "ipfix_names.h"

#ifdef __cplusplus
extern "C" {
#endif

static const struct ipfix_identifier IPFIXTAB[] = {
	{ "RESERVED", 0, 0 },
	{ "inoctetdeltacount", 1, 8 },
	{ "inpacketdeltacount", 2, 8 },
	{ "totalflowcount", 3, 8 },
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


/* ANSI-C code produced by gperf version 2.7 */
/* Command-line: gperf -D -C -t -L ANSI-C  */
struct ipfix_midentifier {
	char *name;
	uint16_t id;
};

#define TOTAL_KEYWORDS 155
#define MIN_WORD_LENGTH 8
#define MAX_WORD_LENGTH 24
#define MIN_HASH_VALUE 8
#define MAX_HASH_VALUE 156
/* maximum key range = 149, duplicates = 80 */

static unsigned int hash (register const char *str, register unsigned int len)
{
	static const unsigned char asso_values[] = {
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157,  15,   5,
		10,  45,   0,  20,  15,  30,  35,  40, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157,  70,  90,
		0,  30, 110, 157, 100,  78, 157,  40,  70,   0,
		20,  15,  25, 157,   0,  55,  15, 157, 157, 157,
		5,   0, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157, 157, 157, 157, 157,
		157, 157, 157, 157, 157, 157
	};

	return len + asso_values[(unsigned char)str[len - 1]] + asso_values[(unsigned char)str[0]];
}

#ifdef __GNUC__
__inline
#endif
const struct ipfix_midentifier * in_word_set (register const char *str, register unsigned int len)
{
	static const struct ipfix_midentifier wordlist[] =
	{
		{"reserved", 0},
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
		{"mplslabelstackentry4", 73},
		{"mplslabelstackentry1", 70},
		{"destinationipv4prefix", 45},
		{"mplslabelstackentry2", 71},
		{"optiontemplateid", 1024},
		{"mplslabelstackentry6", 75},
		{"mplslabelstackentry10", 79},
		{"droppedoctetdeltacount", 132},
		{"droppedoctettotalcount", 134},
		{"droppedpacketdeltacount", 133},
		{"droppedpackettotalcount", 135},
		{"destinationtransportport", 11},
		{"mplslabelstackentry5", 74},
		{"protocolidentifier", 4},
		{"totalflowcount", 3},
		{"octettotalcount", 85},
		{"octetdeltacount", 138},
		{"mplstoplabeltype", 46},
		{"outoctetdeltacount", 23},
		{"outpacketdeltacount", 24},
		{"mplslabelstackentry7", 76},
		{"outmulticastoctetcount", 20},
		{"outmulticastpacketcount", 19},
		{"mplslabelstackentry8", 77},
		{"packettotalcount", 86},
		{"packetdeltacount", 139},
		{"destinationipv4mask", 13},
		{"destinationipv6mask", 30},
		{"mplslabelstackentry9", 78},
		{"exportedflowcount", 42},
		{"exportedoctetcount", 40},
		{"exportedpacketcount", 41},
		{"mplslabelstackentry3", 72},
		{"packetsample", 1026},
		{"sequencenumber", 1025},
		{"samplingalgorithm", 1027},
		{"samplingprobability", 1035},
		{"egressinterface", 14},
		{"sourceipv4prefix", 44},
		{"destinationipv4address", 12},
		{"destinationipv6address", 28},
		{"mplstoplabelipv4address", 47},
		{"mplstoplabelipv6address", 140},
		{"minimumttl", 52},
		{"maximumttl", 53},
		{"tcpcontrolbits", 6},
		{"bgpsourceasnumber", 16},
		{"bgpnexthopasnumber", 128},
		{"sourcetransportport", 7},
		{"bgpdestinationasnumber", 17},
		{"samplingpopulation", 1033},
		{"ipnexthopasnumber", 129},
		{"identificationipv4", 54},
		{"samplingsize", 1034},
		{"samplingtimespace", 1032},
		{"exporteripv4address", 130},
		{"samplingpacketspace", 1030},
		{"exporteripv6address", 131},
		{"ipversion", 60},
		{"classofserviceipv4", 5},
		{"sourceipv4mask", 9},
		{"sourceipv6mask", 29},
		{"inoctetdeltacount", 1},
		{"inpacketdeltacount", 2},
		{"igmptype", 33},
		{"minimumpacketlength", 25},
		{"maximumpacketlength", 26},
		{"icmptypecode", 32},
		{"classofserviceipv6", 137},
		{"ingressinterface", 10},
		{"sourcemacaddress", 56},
		{"sourceipv4address", 8},
		{"sourceipv6address", 27},
		{"filteringalgorithm", 1028},
		{"hashfunction", 1037},
		{"flowlabelipv6", 31},
		{"headertype", 1036},
		{"flowactivetimeout", 36},
		{"flowendreason", 136},
		{"flowinactivetimeout", 37},
		{"samplingtimeinterval", 1031},
		{"bgpnexthopipv4address", 18},
		{"bgpnexthopipv6address", 63},
		{"samplingpacketinterval", 1029},
		{"ipv6optionheaders", 64},
		{"flowendtime", 21},
		{"ipnexthopipv4address", 15},
		{"ipnexthopipv6address", 62},
		{"flowcreationtime", 22}
	};

	static const short lookup[] = {
		-1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
		-165, -155,  -66,   -1,   -1,   -1,   -1,   -1,
		-1,   -1,   -1,   -1,   66,   -1,   -1,   -1,
		-1,   67,   68,   -1,   -1,   -1,   69,   70,
		-1,   -1,   -1,   71,   72, -256, -197,   77,
		78,  -80,   -2,   79,   80, -254,   83,   -1,
		84,   85,   86,   -1,   87,   88,   -1,   89,
		-213,  -65,   -2, -226,   94,   -1,   95,   96,
		97,   98,   -1,   99,   -1,  100,  -63,   -2,
		101,   -1,  102,  103,  104, -246, -241,   -1,
		-237,  -46,   -2,   -1,  111,  -48,   -2,  112,
		113,  114,  -50,   -2,  115,  116,   -1,  117,
		118,  119,  -74,   -2,  -82,   -2,  120,   -1,
		-261,  -34,   -3,  124,  125, -268,  128,  129,
		-29,   -2,   -1,   -1,  130,   -1,   -1, -277,
		133,  -24,   -2,  134,  135,   -1,  136, -285,
		139,  -18,   -2,   -1,  140,   -1,   -1,   -1,
		-1,   -1,  141,   -1,  142,   -1,  143,  144,
		145,  146, -304,  149,   -8,   -2,  150,  151,
		-1, -310,   -3,   -2,  154
	};

	if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH) {
		register int key = hash (str, len);

		if(key <= MAX_HASH_VALUE && key >= 0) {
			register int index = lookup[key];

			if(index >= 0) {
				register const char *s = wordlist[index].name;

				if (*str == *s && !strcmp (str + 1, s + 1))
					return &wordlist[index];

			} else if (index < -TOTAL_KEYWORDS) {
				register int offset = - 1 - TOTAL_KEYWORDS - index;
				register const struct ipfix_midentifier *wordptr = &wordlist[TOTAL_KEYWORDS + lookup[offset]];
				register const struct ipfix_midentifier *wordendptr = wordptr + -lookup[offset + 1];

				while (wordptr < wordendptr) {
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
