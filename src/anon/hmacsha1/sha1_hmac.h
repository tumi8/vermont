#ifndef __SHA1_HMAC_H
#define __SHA1_HMAC_H

namespace HmacSha1 {

void sha1_hmac		(char *dest, char *key, size_t klen, char *text, size_t tlen);
void sha1_hmac_96	(char *dest, char *key, size_t klen, char *text, size_t tlen);

#define SHA1HMAC_LEN 20
#define SHA1HMAC96_LEN 12

} // namespace HmacSha1

#endif // __SHA1_HMAC_H
