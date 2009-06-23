// Package: Crypto-PAn 1.0
// File: panonymizer.cpp
// Last Update: April 17, 2002
// Author: Jinliang Fan

#define _PANONYMIZER_CPP_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "panonymizer.h"

//Constructor
PAnonymizer::PAnonymizer(const UINT8 * key) {
  //initialize the 128-bit secret key.
  memcpy(m_key, key, 16);
  //initialize the Rijndael cipher. 
  m_rin.init(Rijndael::ECB, Rijndael::Encrypt, key, Rijndael::Key16Bytes);
  //initialize the 128-bit secret pad. The pad is encrypted before being used for padding.
  m_rin.blockEncrypt(key + 16, 128, m_pad);  
}

//Destructor
PAnonymizer::~PAnonymizer() {
}

//Anonymization funtion
UINT32 PAnonymizer::anonymize(const UINT32 orig_addr) {
    UINT8 rin_output[16];
    UINT8 rin_input[16];

    UINT32 result = 0;
    UINT32 first4bytes_pad, first4bytes_input;
    int pos;

    memcpy(rin_input, m_pad, 16);
    first4bytes_pad = (((UINT32) m_pad[0]) << 24) + (((UINT32) m_pad[1]) << 16) +
	(((UINT32) m_pad[2]) << 8) + (UINT32) m_pad[3]; 

    // For each prefixes with length from 0 to 31, generate a bit using the Rijndael cipher,
    // which is used as a pseudorandom function here. The bits generated in every rounds
    // are combineed into a pseudorandom one-time-pad.
    for (pos = 0; pos <= 31 ; pos++) { 

	//Padding: The most significant pos bits are taken from orig_addr. The other 128-pos 
        //bits are taken from m_pad. The variables first4bytes_pad and first4bytes_input are used
	//to handle the annoying byte order problem.
	if (pos==0) {
	  first4bytes_input =  first4bytes_pad; 
	}
	else {
	  first4bytes_input = ((orig_addr >> (32-pos)) << (32-pos)) | ((first4bytes_pad<<pos) >> pos);
	}
	rin_input[0] = (UINT8) (first4bytes_input >> 24);
	rin_input[1] = (UINT8) ((first4bytes_input << 8) >> 24);
	rin_input[2] = (UINT8) ((first4bytes_input << 16) >> 24);
	rin_input[3] = (UINT8) ((first4bytes_input << 24) >> 24);

	//Encryption: The Rijndael cipher is used as pseudorandom function. During each 
	//round, only the first bit of rin_output is used.
	m_rin.blockEncrypt(rin_input, 128, rin_output);	

	//Combination: the bits are combined into a pseudorandom one-time-pad
	result |=  (rin_output[0] >> 7) << (31-pos);
    }
    //XOR the orginal address with the pseudorandom one-time-pad
    return result ^ orig_addr;
}
