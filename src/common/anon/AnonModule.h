#ifndef _ANON_MODULE_H_
#define _ANON_MODULE_H_

#include "AnonPrimitive.h"
#include <common/msg.h>
#include <map>
#include <vector>

class AnonMethod 
{
public:
	typedef enum {
		BytewiseHashHmacSha1,
		BytewiseHashSha1,
		ConstOverwrite,
		ContinuousChar,
		HashHmacSha1,
		HashSha1,
		Randomize,
		Shuffle,
		Whitenoise,
		CryptoPan
	} Method;

	static Method stringToMethod(const std::string& m)
	{
		if (m == "BytewiseHashHmacSha1") {
			return BytewiseHashHmacSha1;
		} else if (m == "BytewiseHashSha1") {
                        return BytewiseHashSha1;
                } else if (m == "ConstOverwrite") {
                        return ConstOverwrite;
                } else if (m == "ContinuousChar") {
                        return ContinuousChar;
                }else if (m == "HashHmacSha1") {
                        return HashHmacSha1;
                }else if (m == "HashSha1") {
                        return HashSha1;
                }else if (m == "Randomize") {
                        return Randomize;
                }else if (m == "Shuffle") {
                        return Shuffle;
                }else if (m == "Whitenoise") {
                        return Whitenoise;
                }else if (m == "CryptoPan") {
			return CryptoPan;
		}
		THROWEXCEPTION("Unknown anonymization method");

		// make compile happy
		return BytewiseHashHmacSha1;
	}
};

struct AnonIE {
	int len;
	std::vector<AnonPrimitive*> method;
};

class AnonModule {
public:
	~AnonModule();
	void addAnonymization(uint16_t id, int len, AnonMethod::Method m, const std::string& parameter = "");
	void anonField(uint16_t id, void* data, int len = -1);
protected:
	typedef std::map<uint16_t, AnonIE> MethodMap;
	MethodMap methods;
private:
	AnonPrimitive* createPrimitive(AnonMethod::Method m, const std::string& paramter);
};


#endif
