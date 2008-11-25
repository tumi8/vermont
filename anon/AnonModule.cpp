#include "AnonModule.h"

#include <anon/AnonHashSha1.h>
#include <anon/AnonBytewiseHashHmacSha1.h>
#include <anon/AnonBytewiseHashSha1.h>
#include <anon/AnonConstOverwrite.h>
#include <anon/AnonContinuousChar.h>
#include <anon/AnonWhitenoise.h>
#include <anon/AnonHashHmacSha1.h>
#include <anon/AnonRandomize.h>
#include <anon/AnonShuffle.h>
#include <anon/AnonCryptoPan.h>

#include <ipfixlolib/ipfix_names.h>

AnonModule::~AnonModule()
{
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		for (std::vector<AnonPrimitive*>::iterator j = i->second.method.begin(); j != i->second.method.end(); ++j) {
			delete *j;
		}
	}
}

AnonPrimitive* AnonModule::createPrimitive(AnonMethod::Method m, const std::string& parameter)
{
	AnonPrimitive* ret = 0;
	switch (m) {
	case AnonMethod::HashSha1:
		ret = new AnonHashSha1();
		break;
	case AnonMethod::BytewiseHashSha1:
		ret = new AnonBytewiseHashSha1();
		break;
	case AnonMethod::ContinuousChar:
		ret = new AnonContinuousChar();
		break;
	case AnonMethod::Randomize:
		ret = new AnonRandomize();
		break;
	case AnonMethod::Shuffle:
		ret = new AnonShuffle();
		break;
	case AnonMethod::Whitenoise:
		ret = new AnonWhitenoise(atoi(parameter.c_str()));
		break;
	case AnonMethod::HashHmacSha1:
		ret = new AnonHashHmacSha1(parameter);
		break;
	case AnonMethod::BytewiseHashHmacSha1:
		ret = new AnonBytewiseHashHmacSha1(parameter);
		break;
	case AnonMethod::ConstOverwrite:
		if (parameter.size() != 1) {
			THROWEXCEPTION("AnonConstOverwrite only uses one character as key");
		}
		ret = new AnonConstOverwrite(parameter.c_str()[0]);
		break;
	case AnonMethod::CryptoPan:
		ret = new AnonCryptoPan(parameter);
		break;
	default:
		msg(MSG_FATAL, "AnonPrimitive number %i is unknown", m);
		THROWEXCEPTION("AnonPrimitive number %i is unknown", m);
	}

	return ret;
}

void AnonModule::addAnonymization(uint16_t id, int len,  AnonMethod::Method  methodName, const std::string& parameter)
{
	static const struct ipfix_identifier* ident;
	AnonPrimitive* a = createPrimitive(methodName, parameter);
	if (methods.find(id) != methods.end()) {
		methods[id].method.push_back(a);
	} else {
		AnonIE ie;
		if (len == -1) {
			if (!(ident = ipfix_id_lookup(id))) {
				msg(MSG_ERROR, "Unknown or unsupported id %i detected.", id);
				return;
			}
			len = ident->length;
		}
		ie.len = len;
		ie.method.push_back(a);
		methods[id] = ie;
	}
}

void AnonModule::anonField(uint16_t id, void* data, int len)
{
	if (methods.find(id) == methods.end()) {
		return;
	}
	int l = len==-1?methods[id].len:len;
	for (std::vector<AnonPrimitive*>::iterator i = methods[id].method.begin(); i != methods[id].method.end(); ++i) {
		(*i)->anonimizeBuffer(data, l);
	}
}

