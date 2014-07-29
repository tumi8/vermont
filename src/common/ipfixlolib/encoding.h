#ifndef ENCODING_H
#define ENCODING_H
/*
 This file is part of the ipfixlolib.
 Release under LGPL.

 Header for encoding functions suitable for IPFIX
 Changes by Ronny T. Lampert, 2005-01

 Based upon the original
 by Jan Petranek, University of Tuebingen
 2004-11-12
 jan@petranek.de
 */

#include <stdio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <string.h>
#include <stdint.h>
#include <sys/uio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VENDOR_SPECIFIC 1
#define NOT_VENDOR_SPECIFIC 2

/*
 I took the 64-bit functions from
 http://komssys.sourceforge.net/html/MNRTPNetTypes_8h-source.html
 License is included:
 */


/*
 Copyright (C) 2000 KOM/Darmstadt University of Technology

 You are allowed to use all other parts of the code under the following terms:

 For non-commercial use, code may be used in unmodified form provided
 that this copyright notice and this permission notice appear in
 supporting documentation.

 This software is provided "as is" and without any express or implied
 warranties, including, without limitation, the implied warranty of
 fitness for a particular purpose.

 The code may be subjected to the GNU General Public License, Version 2,014
 and re-distributed under the terms of this license.
 As a special exception, permission is granted to link this code
 with the Qt library and distribute executables, as long as you
 follow the requirements of the GNU GPL in regard to all of the
 software in the executable aside from Qt.

 Commercial use other than under the terms of the GNU General Public
 License is allowed only after express negotiation of conditions
 with the authors.
*/


/* both assumes little-endianness */
uint64_t htonll(uint64_t number);
uint64_t ntohll(uint64_t number);

/*
 * Write an octet
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the octet
 */
int write_octet ( char** p_pos, char* p_end, uint8_t n);

/*
 * Write an unsigned 16-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_unsigned16 ( char** p_pos, char* p_end, uint16_t n);

/*
 * Write an unsigned 32-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_unsigned32 ( char** p_pos,  char* p_end, uint32_t n);

/*
 * Write an unsigned 64-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_unsigned64 ( char** p_pos,  char* p_end, uint64_t n);

/*
 * not implemented yet!
 * Write a 32-bit IEEE float number
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * f the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_float32 ( char** p_base,  char* p_end, float f);


/*
 * Write a boolean
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * b the boolean
 * hint: as required by IPFIX, this bit will be cast to a byte!
 */
int write_boolean ( char** p_pos, char* p_end, char b);

/*
 * Read an octet
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the octet
 */
uint8_t read_octet ( char** p_pos, char* p_end);


/*
 * Read an unsigned 16-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint16_t read_unsigned16 ( char** p_pos, char* p_end);


/*
 * Read an unsigned 32-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint32_t read_unsigned32 ( char** p_pos, char* p_end);

/*
 * not implemented yet!
 * Read a 32-bit IEEE float number
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * f the number (in host-byte order. This function
 * will convert it to network byte order).
 */
float read_float32 ( char** p_base,  char* p_end);

/*
 * Read an unsigned 64-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint64_t read_unsigned64 ( char** p_pos, char* p_end);


/*
 * Write the Vendor-specific extension bit and
 * the and the field ID in an unsigned 16-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * fieldID the fieldID (in host-byte order. This function
 * will convert it to network byte order).
 * char vendor_specific (either VENDOR_SPECIFIC or NOT_VENDOR_SPECIFIC
 */
int write_extension_and_fieldID(char **p_pos, char *p_end, uint16_t fieldID);

/*
 * Read the Vendor-specific extension bit and
 * the and the field ID in an unsigned 16-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data read from written to.
 * It will NOT! be incremented by
 * the amount of read data.
 * (call read_fieldID afterwards!)
 * returns VENDOR_SPECIFIC or NOT_VENDOR_SPECIFIC or, in case
 * of an error, -1
 */
int read_extension_bit ( char** p_pos, char* p_end);

/*
 * Read the field ID. Call this after read_extension_bit!
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of read data.
 * returns the fieldID
 */
uint16_t read_fieldID ( char** p_pos, char* p_end);



/*
 * Read an array of octets
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * p_end the pointer to the end of memory available after p_pos
 * octets the array of bytes
 * length the number of bytes to write
 * note: this will prepend a Variable Length Information Element
 * as specified by
 * draft-ietf-ipfix-protocol-05.txt
 * returns the length or -1 on error.
 */
int read_octet_array ( char** p_pos,  char* p_end, char* p_output);


/*
 * Write an array of octets
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * p_end the pointer to the end of memory available after p_pos
 * octets the array of bytes
 * length the number of bytes to write
 * note: this will prepend a Variable Length Information Element
 * as specified by
 * draft-ietf-ipfix-protocol-05.txt
 */
/* int write_octet_array ( char** p_pos,  char* p_end, char* octets, int length); */



/*
 * Write an ipv4Address
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n is the IP-Address (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_ipv4Address ( char** p_pos,  char* p_end, uint32_t n);

/*
 * Read an ipv4Address
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the IP-Address in host byte order
 */
uint32_t read_ipv4Address ( char** p_pos,  char* p_end);


#ifdef __cplusplus
}
#endif

#endif
