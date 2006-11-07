#include <string.h>
#include "ipfix_names.h"

#ifdef __cplusplus
extern "C" {
#endif

static const struct ipfix_identifier IPFIXTAB[] = {
	{ "reserved", 0, 0 },
	{ "octetdeltacount", 1, 8 },
	{ "packetdeltacount", 2, 8 },
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
	{ "postmcastpacketdeltacount", 19, 8 },
	{ "postmcastoctetdeltacount", 20, 8 },
	{ "flowendsysuptime", 21, 4 },
	{ "flowstartsysuptime", 22, 4 },
	{ "postoctetdeltacount", 23, 8 },
	{ "postpacketdeltacount", 24, 8 },
	{ "minimumpacketlength", 25, 2 },
	{ "maximumpacketlength", 26, 2 },
	{ "sourceipv6address", 27, 8 },
	{ "destinationipv6address", 28, 8 },
	{ "sourceipv6mask", 29, 1 },
	{ "destinationipv6mask", 30, 1 },
	{ "flowlabelipv6", 31, 4 },
	{ "icmptypecodeipv4", 32, 2 },
	{ "igmptype", 33, 1 },
	{ "reserved", 34, 0 },
	{ "reserved", 35, 0 },
	{ "flowactivetimeout", 36, 2 },
	{ "flowinactivetimeout", 37, 2 },
	{ "reserved", 38, 0 },
	{ "reserved", 39, 0 },
	{ "exportedoctettotalcount", 40, 8 },
	{ "exportedmessagetotalcount", 41, 8 },
	{ "exportedflowtotalcount", 42, 8 },
	{ "reserved", 43, 0 },
	{ "sourceipv4prefix", 44, 4 },
	{ "destinationipv4prefix", 45, 4 },
	{ "mplstoplabeltype", 46, 1 },
	{ "mplstoplabelipv4address", 47, 4 },
	{ "reserved", 48, 0 },
	{ "reserved", 49, 0 },
	{ "reserved", 50, 0 },
	{ "reserved", 51, 0 },
	{ "minimumttl", 52, 1 },
	{ "maximumttl", 53, 1 },
	{ "identificationipv4", 54, 2 },
	{ "postclassofserviceipv4", 55, 1 },
	{ "sourcemacaddress", 56, 0 },
	{ "postdestinationmacaddress", 57, 0 },
	{ "vlanid", 58, 2 },
	{ "postvlanid", 59, 2 },
	{ "ipversion", 60, 1 },
	{ "reserved", 61, 0 },
	{ "ipnexthopipv6address", 62, 8 },
	{ "bgpnexthopipv6address", 63, 8 },
	{ "ipv6extensionheaders", 64, 4 },
	{ "reserved", 65, 0 },
	{ "reserved", 66, 0 },
	{ "reserved", 67, 0 },
	{ "reserved", 68, 0 },
	{ "reserved", 69, 0 },
	{ "mplstoplabelstackentry", 70, 4 },
	{ "mplslabelstackentry2", 71, 4 },
	{ "mplslabelstackentry3", 72, 4 },
	{ "mplslabelstackentry4", 73, 4 },
	{ "mplslabelstackentry5", 74, 4 },
	{ "mplslabelstackentry6", 75, 4 },
	{ "mplslabelstackentry7", 76, 4 },
	{ "mplslabelstackentry8", 77, 4 },
	{ "mplslabelstackentry9", 78, 4 },
	{ "mplslabelstackentry10", 79, 4 },
	{ "destinationmacaddress", 80, 0 },
	{ "postsourcemacaddress", 81, 0 },
	{ "reserved", 82, 0 },
	{ "reserved", 83, 0 },
	{ "reserved", 84, 0 },
	{ "octettotalcount", 85, 8 },
	{ "packettotalcount", 86, 8 },
	{ "reserved", 87, 0 },
	{ "fragmentoffsetipv4", 88, 2 },
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
	{ "bgpnextadjacentasnumber", 128, 2 },
	{ "bgpprevadjacentasnumber", 129, 2 },
	{ "exporteripv4address", 130, 4 },
	{ "exporteripv6address", 131, 8 },
	{ "droppedoctetdeltacount", 132, 8 },
	{ "droppedpacketdeltacount", 133, 8 },
	{ "droppedoctettotalcount", 134, 8 },
	{ "droppedpackettotalcount", 135, 8 },
	{ "flowendreason", 136, 1 },
	{ "classofserviceipv6", 137, 1 },
	{ "postclassofserviceipv6", 138, 1 },
	{ "icmptypecodeipv6", 139, 2 },
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
	{ "destinationipv6prefix", 169, 8 },
	{ "sourceipv6prefix", 170, 8 },
	{ "postoctettotalcount", 171, 8 },
	{ "postpackettotalcount", 172, 8 },
	{ "flowkeyindicator", 173, 8 },
	{ "postmcastpackettotalcount", 174, 8 },
	{ "postmcastoctettotalcount", 175, 8 },
	{ "icmptypeipv4", 176, 1 },
	{ "icmpcodeipv4", 177, 1 },
	{ "icmptypeipv6", 178, 1 },
	{ "icmpcodeipv6", 179, 1 },
	{ "udpsourceport", 180, 2 },
	{ "udpdestinationport", 181, 2 },
	{ "tcpsourceport", 182, 2 },
	{ "tcpdestinationport", 183, 2 },
	{ "tcpsequencenumber", 184, 4 },
	{ "tcpacknowledgementnumber", 185, 4 },
	{ "tcpwindowsize", 186, 2 },
	{ "tcpurgentpointer", 187, 2 },
	{ "tcpheaderlength", 188, 2 },
	{ "ipheaderlength", 189, 1 },
	{ "totallengthipv4", 190, 2 },
	{ "payloadlengthipv6", 191, 4 },
	{ "iptimetolive", 192, 1 },
	{ "nextheaderipv6", 193, 1 },
	{ "ipclassofservice", 194, 1 },
	{ "ipdiffservcodepoint", 195, 1 },
	{ "ipprecedence", 196, 1 },
	{ "fragmentflagsipv4", 197, 1 },
	{ "octetdeltasumofsquares", 198, 8 },
	{ "octettotalsumofsquares", 199, 8 },
	{ "mplstoplabelttl", 200, 1 },
	{ "mplslabelstacklength", 201, 4 },
	{ "mplslabelstackdepth", 202, 4 },
	{ "mplstoplabelexp", 203, 1 },
	{ "ippayloadlength", 204, 8 },
	{ "udpmessagelength", 205, 2 },
	{ "ismulticast", 206, 1 },
	{ "internetheaderlengthipv4", 207, 1 },
	{ "ipv4options", 208, 4 },
	{ "tcpoptions", 209, 8 },
	{ "paddingoctets", 210, 0 },
	{ "reserved", 211, 0 },
	{ "reserved", 212, 0 },
	{ "headerlengthipv4", 213, 1 },
	{ "mplspayloadlength", 214, 4 },
};


#define PSAMP_STARTING_ID 300

static const struct ipfix_identifier PSAMPTAB[] = {
	{ "selectorid", 300, 2 },
	{ "selectorinputsequencenumber", 301, 4 },
	{ "selectoralgorithm", 302, 1 },
	{ "reserved", 303, 0 },
	{ "samplingpacketinterval", 304, 4 },
	{ "samplingpacketspace", 305, 4 },
	{ "samplingtimeinterval", 306, 4 },
	{ "samplingtimespace", 307, 4 },
	{ "samplingpopulation", 308, 4 },
	{ "samplingsize", 309, 4 },
	{ "samplingprobabilityn", 310, 4 },
	{ "samplingprobabilitym", 311, 4 },
	{ "reserved", 312, 0 },
	{ "ipheaderpacketsection", 313, 0 },
	{ "ippayloadpacketsection", 314, 0 },
	{ "l2headerpacketsection", 315, 0 },
	{ "l2payloadpacketsection", 316, 0 },
	{ "mplslabelstacksection", 317, 0 },
	{ "mplspayloadpacketsection", 318, 0 },
	{ "meteringprocesssid", 319, 1 },
	{ "observationpointid", 320, 1 },
	{ "associationsid", 321, 1 },
	{ "reserved", 322, 0 },
	{ "selectortype", 323, 1 },
	{ "packetsobserved", 324, 1 },
	{ "packetsselected", 325, 1 },
	{ "accuracy", 326, 1 },
	{ "reserved", 327, 0 },
	{ "reserved", 328, 0 },
	{ "reserved", 329, 0 },
};



/* ANSI-C code produced by gperf version 3.0.1 */
/* Command-line: gperf -D -C -t -L ANSI-C gperf  */
/* Computed positions: -k'1-2,5,10,12-16,$' */

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

#line 1 "gperf"
struct ipfix_midentifier { char *name; uint16_t id; };

#define TOTAL_KEYWORDS 245
#define MIN_WORD_LENGTH 6
#define MAX_WORD_LENGTH 27
#define MIN_HASH_VALUE 30
#define MAX_HASH_VALUE 631
/* maximum key range = 602, duplicates = 67 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632,   0, 632,
       10,  30,  60,  20, 100,  15,   5,   0, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632,   5,  65,  10,
        5,   0,  50, 190, 105,  40,   5, 105,  20,  70,
        0,  10,   0,  15,  25,   5,   0, 110,  55,  95,
      180, 145,  10, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632, 632, 632, 632, 632,
      632, 632, 632, 632, 632, 632
    };
  register int hval = len;

  switch (hval)
    {
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
      case 6:
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      /*FALLTHROUGH*/
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
#endif
const struct ipfix_midentifier *
in_word_set (register const char *str, register unsigned int len)
{
  static const struct ipfix_midentifier wordlist[] =
    {
#line 212 "gperf"
      {"tcpoptions", 209},
#line 218 "gperf"
      {"selectorid", 300},
#line 152 "gperf"
      {"sourceid", 149},
#line 148 "gperf"
      {"templateid", 145},
#line 171 "gperf"
      {"notsentoctettotalcount", 168},
#line 199 "gperf"
      {"ipprecedence", 196},
#line 170 "gperf"
      {"notsentpackettotalcount", 167},
#line 185 "gperf"
      {"tcpsourceport", 182},
#line 243 "gperf"
      {"packetsselected", 325},
#line 145 "gperf"
      {"portid", 142},
#line 3 "gperf"
      {"reserved", 0},
#line 247 "gperf"
      {"reserved", 329},
#line 246 "gperf"
      {"reserved", 328},
#line 245 "gperf"
      {"reserved", 327},
#line 240 "gperf"
      {"reserved", 322},
#line 230 "gperf"
      {"reserved", 312},
#line 221 "gperf"
      {"reserved", 303},
#line 215 "gperf"
      {"reserved", 212},
#line 214 "gperf"
      {"reserved", 211},
#line 130 "gperf"
      {"reserved", 127},
#line 129 "gperf"
      {"reserved", 126},
#line 128 "gperf"
      {"reserved", 125},
#line 127 "gperf"
      {"reserved", 124},
#line 126 "gperf"
      {"reserved", 123},
#line 125 "gperf"
      {"reserved", 122},
#line 124 "gperf"
      {"reserved", 121},
#line 123 "gperf"
      {"reserved", 120},
#line 122 "gperf"
      {"reserved", 119},
#line 121 "gperf"
      {"reserved", 118},
#line 120 "gperf"
      {"reserved", 117},
#line 119 "gperf"
      {"reserved", 116},
#line 118 "gperf"
      {"reserved", 115},
#line 117 "gperf"
      {"reserved", 114},
#line 116 "gperf"
      {"reserved", 113},
#line 115 "gperf"
      {"reserved", 112},
#line 114 "gperf"
      {"reserved", 111},
#line 113 "gperf"
      {"reserved", 110},
#line 112 "gperf"
      {"reserved", 109},
#line 111 "gperf"
      {"reserved", 108},
#line 110 "gperf"
      {"reserved", 107},
#line 109 "gperf"
      {"reserved", 106},
#line 108 "gperf"
      {"reserved", 105},
#line 107 "gperf"
      {"reserved", 104},
#line 106 "gperf"
      {"reserved", 103},
#line 105 "gperf"
      {"reserved", 102},
#line 104 "gperf"
      {"reserved", 101},
#line 103 "gperf"
      {"reserved", 100},
#line 102 "gperf"
      {"reserved", 99},
#line 101 "gperf"
      {"reserved", 98},
#line 100 "gperf"
      {"reserved", 97},
#line 99 "gperf"
      {"reserved", 96},
#line 98 "gperf"
      {"reserved", 95},
#line 97 "gperf"
      {"reserved", 94},
#line 96 "gperf"
      {"reserved", 93},
#line 95 "gperf"
      {"reserved", 92},
#line 94 "gperf"
      {"reserved", 91},
#line 93 "gperf"
      {"reserved", 90},
#line 92 "gperf"
      {"reserved", 89},
#line 90 "gperf"
      {"reserved", 87},
#line 87 "gperf"
      {"reserved", 84},
#line 86 "gperf"
      {"reserved", 83},
#line 85 "gperf"
      {"reserved", 82},
#line 72 "gperf"
      {"reserved", 69},
#line 71 "gperf"
      {"reserved", 68},
#line 70 "gperf"
      {"reserved", 67},
#line 69 "gperf"
      {"reserved", 66},
#line 68 "gperf"
      {"reserved", 65},
#line 64 "gperf"
      {"reserved", 61},
#line 54 "gperf"
      {"reserved", 51},
#line 53 "gperf"
      {"reserved", 50},
#line 52 "gperf"
      {"reserved", 49},
#line 51 "gperf"
      {"reserved", 48},
#line 46 "gperf"
      {"reserved", 43},
#line 42 "gperf"
      {"reserved", 39},
#line 41 "gperf"
      {"reserved", 38},
#line 38 "gperf"
      {"reserved", 35},
#line 37 "gperf"
      {"reserved", 34},
#line 6 "gperf"
      {"reserved", 3},
#line 27 "gperf"
      {"postpacketdeltacount", 24},
#line 211 "gperf"
      {"ipv4options", 208},
#line 137 "gperf"
      {"droppedoctettotalcount", 134},
#line 213 "gperf"
      {"paddingoctets", 210},
#line 84 "gperf"
      {"postsourcemacaddress", 81},
#line 138 "gperf"
      {"droppedpackettotalcount", 135},
#line 63 "gperf"
      {"ipversion", 60},
#line 175 "gperf"
      {"postpackettotalcount", 172},
#line 169 "gperf"
      {"notsentflowtotalcount", 166},
#line 135 "gperf"
      {"droppedoctetdeltacount", 132},
#line 189 "gperf"
      {"tcpwindowsize", 186},
#line 10 "gperf"
      {"sourcetransportport", 7},
#line 209 "gperf"
      {"ismulticast", 206},
#line 227 "gperf"
      {"samplingsize", 309},
#line 174 "gperf"
      {"postoctettotalcount", 171},
#line 62 "gperf"
      {"postvlanid", 59},
#line 136 "gperf"
      {"droppedpacketdeltacount", 133},
#line 26 "gperf"
      {"postoctetdeltacount", 23},
#line 144 "gperf"
      {"linecardid", 141},
#line 59 "gperf"
      {"sourcemacaddress", 56},
#line 186 "gperf"
      {"tcpdestinationport", 183},
#line 139 "gperf"
      {"flowendreason", 136},
#line 239 "gperf"
      {"associationsid", 321},
#line 9 "gperf"
      {"tcpcontrolbits", 6},
#line 225 "gperf"
      {"samplingtimespace", 307},
#line 154 "gperf"
      {"flowendseconds", 151},
#line 14 "gperf"
      {"destinationtransportport", 11},
#line 242 "gperf"
      {"packetsobserved", 324},
#line 160 "gperf"
      {"flowendnanoseconds", 157},
#line 23 "gperf"
      {"postmcastoctetdeltacount", 20},
#line 151 "gperf"
      {"flowid", 148},
#line 178 "gperf"
      {"postmcastoctettotalcount", 175},
#line 159 "gperf"
      {"flowstartnanoseconds", 156},
#line 61 "gperf"
      {"vlanid", 58},
#line 153 "gperf"
      {"flowstartseconds", 150},
#line 150 "gperf"
      {"wlanssid", 147},
#line 158 "gperf"
      {"flowendmicroseconds", 155},
#line 190 "gperf"
      {"tcpurgentpointer", 187},
#line 13 "gperf"
      {"ingressinterface", 10},
#line 11 "gperf"
      {"sourceipv4address", 8},
#line 224 "gperf"
      {"samplingtimeinterval", 306},
#line 5 "gperf"
      {"packetdeltacount", 2},
#line 89 "gperf"
      {"packettotalcount", 86},
#line 4 "gperf"
      {"octetdeltacount", 1},
#line 195 "gperf"
      {"iptimetolive", 192},
#line 183 "gperf"
      {"udpsourceport", 180},
#line 223 "gperf"
      {"samplingpacketspace", 305},
#line 60 "gperf"
      {"postdestinationmacaddress", 57},
#line 237 "gperf"
      {"meteringprocesssid", 319},
#line 233 "gperf"
      {"l2headerpacketsection", 315},
#line 241 "gperf"
      {"selectortype", 323},
#line 88 "gperf"
      {"octettotalcount", 85},
#line 83 "gperf"
      {"destinationmacaddress", 80},
#line 156 "gperf"
      {"flowendmilliseconds", 153},
#line 231 "gperf"
      {"ipheaderpacketsection", 313},
#line 179 "gperf"
      {"icmptypeipv4", 176},
#line 238 "gperf"
      {"observationpointid", 320},
#line 30 "gperf"
      {"sourceipv6address", 27},
#line 149 "gperf"
      {"wlanchannelid", 146},
#line 198 "gperf"
      {"ipdiffservcodepoint", 195},
#line 67 "gperf"
      {"ipv6extensionheaders", 64},
#line 180 "gperf"
      {"icmpcodeipv4", 177},
#line 244 "gperf"
      {"accuracy", 326},
#line 56 "gperf"
      {"maximumttl", 53},
#line 197 "gperf"
      {"ipclassofservice", 194},
#line 219 "gperf"
      {"selectorinputsequencenumber", 301},
#line 184 "gperf"
      {"udpdestinationport", 181},
#line 210 "gperf"
      {"internetheaderlengthipv4", 207},
#line 24 "gperf"
      {"flowendsysuptime", 21},
#line 146 "gperf"
      {"meteringprocessid", 143},
#line 203 "gperf"
      {"mplstoplabelttl", 200},
#line 7 "gperf"
      {"protocolidentifier", 4},
#line 57 "gperf"
      {"identificationipv4", 54},
#line 236 "gperf"
      {"mplspayloadpacketsection", 318},
#line 177 "gperf"
      {"postmcastpackettotalcount", 174},
#line 157 "gperf"
      {"flowstartmicroseconds", 154},
#line 222 "gperf"
      {"samplingpacketinterval", 304},
#line 22 "gperf"
      {"postmcastpacketdeltacount", 19},
#line 8 "gperf"
      {"classofserviceipv4", 5},
#line 55 "gperf"
      {"minimumttl", 52},
#line 226 "gperf"
      {"samplingpopulation", 308},
#line 81 "gperf"
      {"mplslabelstackentry9", 78},
#line 82 "gperf"
      {"mplslabelstackentry10", 79},
#line 58 "gperf"
      {"postclassofserviceipv4", 55},
#line 36 "gperf"
      {"igmptype", 33},
#line 80 "gperf"
      {"mplslabelstackentry8", 77},
#line 235 "gperf"
      {"mplslabelstacksection", 317},
#line 15 "gperf"
      {"destinationipv4address", 12},
#line 74 "gperf"
      {"mplslabelstackentry2", 71},
#line 161 "gperf"
      {"flowstartdeltamicroseconds", 158},
#line 43 "gperf"
      {"exportedoctettotalcount", 40},
#line 79 "gperf"
      {"mplslabelstackentry7", 76},
#line 166 "gperf"
      {"observedflowtotalcount", 163},
#line 25 "gperf"
      {"flowstartsysuptime", 22},
#line 77 "gperf"
      {"mplslabelstackentry5", 74},
#line 155 "gperf"
      {"flowstartmilliseconds", 152},
#line 147 "gperf"
      {"exportingprocessid", 144},
#line 193 "gperf"
      {"totallengthipv4", 190},
#line 181 "gperf"
      {"icmptypeipv6", 178},
#line 75 "gperf"
      {"mplslabelstackentry3", 72},
#line 140 "gperf"
      {"classofserviceipv6", 137},
#line 162 "gperf"
      {"flowenddeltamicroseconds", 159},
#line 228 "gperf"
      {"samplingprobabilityn", 310},
#line 176 "gperf"
      {"flowkeyindicator", 173},
#line 182 "gperf"
      {"icmpcodeipv6", 179},
#line 17 "gperf"
      {"egressinterface", 14},
#line 141 "gperf"
      {"postclassofserviceipv6", 138},
#line 40 "gperf"
      {"flowinactivetimeout", 37},
#line 31 "gperf"
      {"destinationipv6address", 28},
#line 165 "gperf"
      {"flowdurationmicroseconds", 162},
#line 35 "gperf"
      {"icmptypecodeipv4", 32},
#line 168 "gperf"
      {"ignoredoctettotalcount", 165},
#line 164 "gperf"
      {"flowdurationmilliseconds", 161},
#line 76 "gperf"
      {"mplslabelstackentry4", 73},
#line 201 "gperf"
      {"octetdeltasumofsquares", 198},
#line 167 "gperf"
      {"ignoredpackettotalcount", 164},
#line 29 "gperf"
      {"maximumpacketlength", 26},
#line 220 "gperf"
      {"selectoralgorithm", 302},
#line 187 "gperf"
      {"tcpsequencenumber", 184},
#line 202 "gperf"
      {"octettotalsumofsquares", 199},
#line 49 "gperf"
      {"mplstoplabeltype", 46},
#line 234 "gperf"
      {"l2payloadpacketsection", 316},
#line 91 "gperf"
      {"fragmentoffsetipv4", 88},
#line 12 "gperf"
      {"sourceipv4mask", 9},
#line 39 "gperf"
      {"flowactivetimeout", 36},
#line 131 "gperf"
      {"bgpnextadjacentasnumber", 128},
#line 232 "gperf"
      {"ippayloadpacketsection", 314},
#line 133 "gperf"
      {"exporteripv4address", 130},
#line 194 "gperf"
      {"payloadlengthipv6", 191},
#line 28 "gperf"
      {"minimumpacketlength", 25},
#line 78 "gperf"
      {"mplslabelstackentry6", 75},
#line 50 "gperf"
      {"mplstoplabelipv4address", 47},
#line 229 "gperf"
      {"samplingprobabilitym", 311},
#line 73 "gperf"
      {"mplstoplabelstackentry", 70},
#line 205 "gperf"
      {"mplslabelstackdepth", 202},
#line 132 "gperf"
      {"bgpprevadjacentasnumber", 129},
#line 206 "gperf"
      {"mplstoplabelexp", 203},
#line 188 "gperf"
      {"tcpacknowledgementnumber", 185},
#line 45 "gperf"
      {"exportedflowtotalcount", 42},
#line 32 "gperf"
      {"sourceipv6mask", 29},
#line 204 "gperf"
      {"mplslabelstacklength", 201},
#line 20 "gperf"
      {"bgpdestinationasnumber", 17},
#line 134 "gperf"
      {"exporteripv6address", 131},
#line 142 "gperf"
      {"icmptypecodeipv6", 139},
#line 143 "gperf"
      {"mplstoplabelipv6address", 140},
#line 34 "gperf"
      {"flowlabelipv6", 31},
#line 196 "gperf"
      {"nextheaderipv6", 193},
#line 163 "gperf"
      {"systeminittimemilliseconds", 160},
#line 16 "gperf"
      {"destinationipv4mask", 13},
#line 21 "gperf"
      {"bgpnexthopipv4address", 18},
#line 217 "gperf"
      {"mplspayloadlength", 214},
#line 48 "gperf"
      {"destinationipv4prefix", 45},
#line 18 "gperf"
      {"ipnexthopipv4address", 15},
#line 44 "gperf"
      {"exportedmessagetotalcount", 41},
#line 33 "gperf"
      {"destinationipv6mask", 30},
#line 191 "gperf"
      {"tcpheaderlength", 188},
#line 66 "gperf"
      {"bgpnexthopipv6address", 63},
#line 172 "gperf"
      {"destinationipv6prefix", 169},
#line 65 "gperf"
      {"ipnexthopipv6address", 62},
#line 192 "gperf"
      {"ipheaderlength", 189},
#line 208 "gperf"
      {"udpmessagelength", 205},
#line 200 "gperf"
      {"fragmentflagsipv4", 197},
#line 19 "gperf"
      {"bgpsourceasnumber", 16},
#line 47 "gperf"
      {"sourceipv4prefix", 44},
#line 173 "gperf"
      {"sourceipv6prefix", 170},
#line 207 "gperf"
      {"ippayloadlength", 204},
#line 216 "gperf"
      {"headerlengthipv4", 213}
    };

  static const short lookup[] =
    {
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,    0,   -1,
        -1,   -1,   -1,    1,   -1,   -1,    2,   -1,
         3,   -1,   -1,   -1,   -1,   -1,   -1,    4,
        -1,   -1,   -1,   -1,    5,    6,   -1,   -1,
        -1,   -1,    7,   -1,    8,    9,   -1, -317,
        -1,   78,   79,   80,   81,   -1,   82, -235,
       -68,   83,   84,   85,   86,   87,   88,   89,
        -1,   90,   91,   -1,   92,   93,   -1,   -1,
        94,   95,   96,   97,   -1,   98,   -1,   -1,
        -1,   -1,   99,  100,   -1,   -1,   -1,   -1,
       101,   -1,   -1,  102,   -1,  103,   -1,   -1,
        -1,   -1,  104,  105,   -1,   -1,  106,  107,
        -1,  108,   -1,   -1,  109,  110,  111,   -1,
        -1,   -1,   -1,  112,   -1,  113,   -1,   -1,
        -1,   -1,   -1,  114,   -1,  115,   -1,   -1,
        -1,   -1,  116,  117,   -1,   -1,  118,  119,
        -1,   -1,   -1,   -1,  120,   -1,   -1,   -1,
       121,   -1,  122,  123,  124,  125,   -1,   -1,
       126,   -1,   -1,  127,  128,   -1,   -1,  129,
       130,   -1,   -1,  131,   -1,  132,  133,  134,
        -1,   -1,   -1,  135,  136,  137,  138,   -1,
       139,  140,   -1,  141,  142,  143,  144,  145,
        -1,  146,  147,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,  148,   -1,   -1,  149,   -1,   -1,
        -1,   -1,  150,  151,  152,  153,  154,   -1,
        -1,  155,   -1,   -1,  156,   -1,  157,   -1,
        -1,  158,   -1,  159,  160,  161,  162,   -1,
       163,  164,  165,   -1,   -1,  166,  167,   -1,
       168,   -1,  169,   -1,  170,  171,   -1,  172,
       173,   -1,  174,   -1,  175,   -1,  176,   -1,
        -1,  177,   -1,   -1,  178,  179,  180,  181,
       182,   -1,   -1,  183,   -1,  184,   -1,  185,
        -1,   -1,  186,   -1,   -1,   -1,   -1,   -1,
        -1,  187,   -1,  188,  189,   -1,  190,  191,
        -1,  192,  193,  194,   -1,   -1,  195,   -1,
        -1,   -1,   -1,  196,   -1,   -1,   -1,   -1,
       197,   -1,   -1,   -1,  198,  199,  200,  201,
        -1,   -1,  202,  203,   -1,   -1,   -1,  204,
        -1,  205,   -1,   -1,  206,   -1,  207,  208,
        -1,   -1,  209,   -1,  210,   -1,  211,   -1,
       212,   -1,   -1,   -1,  213,   -1,  214,   -1,
        -1,   -1,  215,   -1,   -1,  216,   -1,  217,
       218,   -1,   -1,   -1,   -1,   -1,   -1,  219,
        -1,  220,   -1,  221,   -1,   -1,   -1,   -1,
        -1,   -1,  222,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  223,  224,
        -1,  225,   -1,   -1,  226,   -1,  227,  228,
        -1,   -1,   -1,  229,   -1,   -1,   -1,  230,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,  231,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,  232,  233,  234,   -1,
        -1,   -1,   -1,  235,   -1,   -1,   -1,  236,
        -1,   -1,   -1,  237,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,  238,  239,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,  240,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       241,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,
       242,   -1,   -1,   -1,  243,   -1,   -1,   -1,
        -1,   -1,   -1,   -1,   -1,   -1,   -1,  244
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
        if(ipfix_rangecheck_ipfix(n)) {
		return &IPFIXTAB[n];
	}

	/* ID is PSAMP one */
        if(ipfix_rangecheck_psamp(n)) {
		/* have to normalize */
		n -= PSAMP_STARTING_ID;
		return &PSAMPTAB[n];
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

	if(!(tmp=in_word_set(name, strlen(name)))) {
		/* not found */
		return -1;
	}

	return (int)tmp->id;
}

#ifdef __cplusplus
}
#endif
