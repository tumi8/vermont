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
#include "encoding.h"
#include "common/msg.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * I took the 64-bit functions from
 * http://komssys.sourceforge.net/html/MNRTPNetTypes_8h-source.html
 * License is included:
 */


/* Copyright (C) 2000 KOM/Darmstadt University of Technology
 *
 * You are allowed to use all other parts of the code under the following terms:
 *
 * For non-commercial use, code may be used in unmodified form provided
 * that this copyright notice and this permission notice appear in
 * supporting documentation.
 *
 * This software is provided "as is" and without any express or implied
 * warranties, including, without limitation, the implied warranty of
 * fitness for a particular purpose.
 *
 * The code may be subjected to the GNU General Public License, Version 2,014
 * and re-distributed under the terms of this license.
 * As a special exception, permission is granted to link this code
 * with the Qt library and distribute executables, as long as you
 * follow the requirements of the GNU GPL in regard to all of the
 * software in the executable aside from Qt.
 *
 * Commercial use other than under the terms of the GNU General Public
 * License is allowed only after express negotiation of conditions
 * with the authors.
 */


// both assumes little-endianness.
uint64_t htonll(uint64_t number)
{
	return ( htonl( (number >> 32) & 0xFFFFFFFF) |
		 ((uint64_t) (htonl(number & 0xFFFFFFFF))  << 32));
}

uint64_t ntohll(uint64_t number)
{
	return ( htonl( (number >> 32) & 0xFFFFFFFF) |
		 ((uint64_t) (htonl(number & 0xFFFFFFFF))  << 32));
}


/*
 * Write an octet
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the octet
 */
int write_octet ( char** p_pos, char* p_end, uint8_t n)
{
	if (p_end < ( *p_pos + sizeof(uint8_t)  ) ) {
		msg(MSG_ERROR, "error in write_octet: buffer too small!");
		return -1;
	}
	uint8_t uint8 = n;
	memcpy ( *p_pos, &uint8, sizeof(uint8_t) );
	*p_pos += sizeof (uint8_t);
	return 0;
}


/*
 * Write an unsigned 16-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_unsigned16 ( char** p_pos, char* p_end, uint16_t n)
{

	if (p_end < ( *p_pos + sizeof(uint16_t)  ) ) {
		msg(MSG_ERROR, "error in write_unsigned16: buffer too small!");
		return -1;
	}
	uint16_t uint16=htons(n);
	memcpy(*p_pos, &uint16, sizeof(uint16_t));
	*p_pos += sizeof(uint16_t);
	return 0;
}


/*
 * Write an unsigned 32-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_unsigned32 ( char** p_pos,  char* p_end, uint32_t n)
{
	if (p_end < ( *p_pos + sizeof(uint32_t)  ) ) {
		msg(MSG_ERROR, "error in write_unsigned32: buffer too small!");
		return -1;
	}
	uint32_t  uint32 = htonl (n);
	memcpy ( *p_pos, &uint32, sizeof(uint32_t) );
	*p_pos += sizeof (uint32_t);
	return 0;
}

/*
 * Write an ipv4Address
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n is the IP-Address the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_ipv4Address ( char** p_pos,  char* p_end, uint32_t n)
{
	return write_unsigned32 (p_pos, p_end, n);
}


/*
 * Write an unsigned 64-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * n the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_unsigned64 ( char** p_pos,  char* p_end, uint64_t n)
{
	if (p_end < ( *p_pos + sizeof(uint64_t)  ) ) {
		msg(MSG_ERROR, "error in write_unsigned64: buffer too small!");
		return -1;
	}
	uint64_t uint64 =  htonll (n);
	memcpy  ( *p_pos, &uint64, sizeof(uint64_t) );
	*p_pos += sizeof (uint64_t);
	return 0;
}


/*
 * Write a 32-bit IEEE float number
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * f the number (in host-byte order. This function
 * will convert it to network byte order).
 */
int write_float32 ( char** p_base,  char* p_end, float f)
{
	if (p_end < ( *p_base + sizeof(float)  ) ) {
		msg(MSG_ERROR, "error in write_float32: buffer too small!");
		return -1;
	}

	// we assume, all we need is to convert the float to network-byte-order
	//  char* p_base = malloc (sizeof (float));
	char* p_float = (char*) &f;
	char* p_pos  = *p_base;
	*(p_pos+3) = *p_float;
	*(p_pos+2) = *(p_float+1);
	*(p_pos+1) = *(p_float+2);
	*p_pos = *(p_float+3);

	/*   int i; */
	/*   for (i=0; i<4; i++) { */
	/*     printf ("p_float_host[%i] : %x \n", i, *(p_float+i));  */

	/*   } */
	/*   for (i=0; i<4; i++) { */
	/*     printf ("p_float_net[%i] : %x \n", i, *(p_pos+i));  */

	/*   } */
	return 0;
}


/*
 * Write a boolean
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * b the boolean
 * hint: as required by IPFIX, this bit will be cast to a byte!
 */
int write_boolean(char **p_pos, char *p_end, char b)
{
	uint8_t uint8 = (uint8_t) b;
	return write_octet (p_pos, p_end, uint8);
}


/*
 * Read an octet
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the octet
 */
uint8_t read_octet(char **p_pos, char *p_end)
{
	if (p_end < ( *p_pos + sizeof(uint8_t)  ) ) {
		msg(MSG_ERROR, "error in read_octet: buffer too small!");
		return -1;
	}

	uint8_t n = (uint8_t) **p_pos;
	*p_pos += sizeof(uint8_t);
	return n;
}


/*
 * Read an unsigned 16-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint16_t read_unsigned16(char **p_pos, char *p_end)
{
	if (p_end < ( *p_pos + sizeof(uint16_t)  ) ) {
		msg(MSG_ERROR, "error in read_unsigned16: buffer too small!");
		return -1;
	}
	// **p_pos is a pointer pointer.
	// first, we dereference the pointer to the real char* data.
	// then, we cast this pointer to a pointer of type uint16_t
	// finally, we dereference that pointer and get the real value
	uint16_t n = *( (uint16_t*) *p_pos);
	// and convert it back to host byte order
	n = ntohs (n);
	*p_pos += sizeof(uint16_t);
	return n;
}


/*
 * Read an unsigned 32-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint32_t read_unsigned32(char **p_pos, char *p_end)
{
	if (p_end < ( *p_pos + sizeof(uint32_t)  ) ) {
		msg(MSG_ERROR, "error in read_unsigned32: buffer too small!");
		return -1;
	}
	// **p_pos is a pointer pointer.
	// first, we dereference the pointeencoding.c:161:r to the real char* data.
	// then, we cast this pointer to a pointer of type uint32_t
	// finally, we dereference that pointer and get the real value
	uint32_t n = *( (uint32_t*) *p_pos);
	*p_pos += sizeof(uint32_t);
	// and convert it back to host byte order
	n = ntohl (n);
	return n;
}


/*
 * Read an ipv4Address
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint32_t read_ipv4Address(char **p_pos, char *p_end)
{
	return read_unsigned32 (p_pos, p_end);
}


/*
 * Read a 32-bit IEEE float number
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is written to. It will be incremented by
 * the amount of written data.
 * returns the float.
 */
float read_float32(char **p_base, char *p_end)
{
	// we assume, all we need is to convert the float to network-byte-order
	if (p_end < ( *p_base + sizeof(float)  ) ) {
		msg(MSG_ERROR, "error in read_float32: buffer too small!");
		return -1;
	}
	float f = 2.1; // initialize the float to some dummy value.
	// we assume, all we need is to convert the float to network-byte-order

	char* p_float= (char*) &f;

	char* p_pos  = *p_base;

	p_float[3]= *p_pos;
	p_float[2] = *(p_pos+1);
	p_float[1]= *(p_pos+2);
	p_float[0] = *(p_pos+3);


	return f;
}


/*
 * Read an unsigned 64-bit integer
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of written data.
 * returns the integer
 */
uint64_t read_unsigned64 ( char** p_pos, char* p_end)
{
	if (p_end < ( *p_pos + sizeof(uint64_t)  ) ) {
		msg(MSG_ERROR, "error in read_unsigned64: buffer too small!");
		return -1;
	}
	// **p_pos is a pointer pointer.
	// first, we dereference the pointer to the real char* data.
	// then, we cast this pointer to a pointer of type uint64_t
	// finally, we dereference that pointer and get the real value
	uint64_t n = *( (uint64_t*) *p_pos);
	*p_pos += sizeof(uint64_t);
	// and convert it back to host byte order
	n = ntohll (n);
	return n;
}


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
int read_octet_array(char **p_pos, char *p_end, char *p_output)
{
	// the length encoded in 1 or 3 bytes?
	uint16_t data_length = 0;
	int first_len;

	first_len=read_octet(p_pos, p_end);
	// simply must work!
	/*   if (first_len == -1) { */
	/*     msg(MSG_ERROR, "error in read_octet_array: reading first length byte failed!");       */
	/*    return -1; */
	/*   } */

	if (first_len < 255) {
		data_length = (uint16_t) first_len;
	} else {
		// read another 2 bytes
		data_length = read_unsigned16 (p_pos, p_end);
		if (data_length < 255 ) {
			msg(MSG_ERROR, "malformed array of octets");
		}
	}
	memcpy (p_output, *p_pos, data_length);
	p_pos += data_length;

	return data_length;
}


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
int write_extension_and_fieldID(char** p_pos, char* p_end, uint16_t fieldID)
{
	uint16_t uint16 = htons(fieldID);
	//uint16_t leading_bit = 1 << 15;

	if (p_end < ( *p_pos + sizeof(uint16_t)  ) ) {
		msg(MSG_ERROR, "error in write_unsigned16: buffer too small!");
		return -1;
	}
	
	memcpy(*p_pos, &uint16, sizeof(uint16_t));
	*p_pos += sizeof (uint16_t);

	return 0;
}


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
int read_extension_bit(char **p_pos, char *p_end)
{
	// a uint16_t, where only the leftmost bit is set.
	// needet for the vendor-Specific bit.
	uint16_t leading_bit = 1 << 15;
	uint16_t n = *( (uint16_t*) *p_pos);

	if (p_end < ( *p_pos + sizeof(uint16_t)  ) ) {
		msg(MSG_ERROR, "error in write_unsigned16: buffer too small!");
		return -1;
	}

	if((n & leading_bit) > 0 ) {
		return VENDOR_SPECIFIC;
	} else {
		return NOT_VENDOR_SPECIFIC;
	}
}


/*
 * Read the field ID.
 * Parameters:
 * p_pos is a pointer to a buffer, where the
 * data is read from. It will be incremented by
 * the amount of read data.
 * returns the fieldID
 */
uint16_t read_fieldID(char **p_pos, char *p_end)
{
	if (p_end < ( *p_pos + sizeof(uint16_t)  ) ) {
		msg(MSG_ERROR, "error in read_unsigned16: buffer too small!");
		return -1;
	}

	// first, we dereference the pointer to the real char* data.
	// then, we cast this pointer to a pointer of type uint16_t
	// finally, we dereference that pointer and get the real value
	uint16_t n = *( (uint16_t*) *p_pos);

	// and convert it back to host byte order
	n = ntohs(n);
	*p_pos += sizeof(uint16_t);

	return n;
}


#ifdef __cplusplus
}
#endif
