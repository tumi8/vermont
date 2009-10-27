#ifndef __SHA1_H
#define __SHA1_H

#include <stdint.h>

namespace Sha1  {

#define SHA1_DIGLEN 20

typedef struct {
    uint32_t state[5];
    uint32_t count[2];
    unsigned char buffer[64];
} SHA1_CTX;

void SHA1Transform	(uint32_t state[5], unsigned char buffer[64]);
void SHA1_Init		(SHA1_CTX* context);
void SHA1_Update	(SHA1_CTX* context, unsigned char* data, unsigned int len);
void SHA1_Final		(unsigned char digest[20], SHA1_CTX* context);

} // namespace Sha1 

#endif /* __SHA1_H */

