//Package: Crypto-PAn 1.0
//File: panonymizer.h
//Last Update: April 11, 2002
//Author: Jinliang Fan

#ifndef _PANONYMIZER_H_
#define _PANONYMIZER_H_

#include "rijndael.h"

class PAnonymizer { //Prefix-preserving anonymizer
 public:
    // Contructor need a 256-bit key
    // The first 128 bits of the key are used as the secret key for rijndael cipher
    // The second 128 bits of the key are used as the secret pad for padding
    PAnonymizer(const UINT8 * key);
    ~PAnonymizer();
 protected:
    UINT8 m_key[16]; //128 bit secret key
    UINT8 m_pad[16]; //128 bit secret pad
    Rijndael m_rin;  //Rijndael cipher as pseudorandom function

 public:
    UINT32 anonymize( const UINT32 orig_addr);   
};

#endif //_PANONYMIZER_H_ 
