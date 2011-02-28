/*
 * Vermont Anonymization Subsystem
 * Copyright (C) 2008 Lothar Braun
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "AnonModule.h"

#include <common/anon/AnonHashSha1.h>
#include <common/anon/AnonBytewiseHashHmacSha1.h>
#include <common/anon/AnonBytewiseHashSha1.h>
#include <common/anon/AnonConstOverwrite.h>
#include <common/anon/AnonContinuousChar.h>
#include <common/anon/AnonWhitenoise.h>
#include <common/anon/AnonHashHmacSha1.h>
#include <common/anon/AnonRandomize.h>
#include <common/anon/AnonShuffle.h>
#include <common/anon/AnonCryptoPan.h>
#include <common/anon/AnonCryptoPanPrefix.h>

#include "common/Misc.h"

#include <common/ipfixlolib/ipfix_names.h>

AnonModule::~AnonModule()
{
	for (MethodMap::iterator i = methods.begin(); i != methods.end(); ++i) {
		for (std::vector<AnonPrimitive*>::iterator j = i->second.primitive.begin(); j != i->second.primitive.end(); ++j) {
			delete *j;
		}
	}
}

AnonPrimitive* AnonModule::createPrimitive(AnonMethod::Method m, const std::string& parameter, std::vector<map_info> mapping)
{
	AnonPrimitive* ret = 0;
	char buffer[32];
	char c;

	bool isHex = isHexString(parameter);

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
		if (parameter.size() != 1 || (isHex && parameter.size() != 4)) {
			THROWEXCEPTION("AnonConstOverwrite only uses one character as key");
		}
		c = parameter.c_str()[0];
		if (isHex) {
			if (convHexToBinary(parameter, &c, 1)!=1) {
				THROWEXCEPTION("Failed to convert hexadecimal key parameter '%s' to binary (one byte required)!", parameter.c_str());
			}
		}
		ret = new AnonConstOverwrite(parameter.c_str()[0]);
		break;
	case AnonMethod::CryptoPan:
		if (parameter.length()!=32)
			if (isHex && parameter.length() != 66)
				THROWEXCEPTION("CryptoPAN key *MUST* have exactly 32 characters!");

		if (isHex) {
			if (convHexToBinary(parameter, buffer, 32)!=32) {
				THROWEXCEPTION("Failed to convert hexadecimal key parameter '%s' to binary (32 bytes required)!", parameter.c_str());
			}
		} else {
			memcpy(buffer, parameter.c_str(), 32);
		}
		ret = new AnonCryptoPan(buffer);
		break;
    case AnonMethod::CryptoPanPrefix:
        if (parameter.length()!=32)
            if (isHex && parameter.length() != 66)
                THROWEXCEPTION("CryptoPAN key *MUST* have exactly 32 characters!");

        if (isHex) {
            if (convHexToBinary(parameter, buffer, 32)!=32) {
                THROWEXCEPTION("Failed to convert hexadecimal key parameter '%s' to binary (32 bytes required)!", parameter.c_str());
            }
        } else {
            memcpy(buffer, parameter.c_str(), 32);
        }
        ret = new AnonCryptoPanPrefix(buffer, mapping);
        break;
	default:
		msg(MSG_FATAL, "AnonPrimitive number %i is unknown", m);
		THROWEXCEPTION("AnonPrimitive number %i is unknown", m);
	}

	return ret;
}

void AnonModule::addAnonymization(InformationElement::IeInfo id, int len, AnonMethod::Method methodName, std::vector<map_info> mapping, const std::string& parameter)
{
	static const struct ipfix_identifier* ident;
	AnonPrimitive* a = createPrimitive(methodName, parameter, mapping);
	if (methods.find(id) != methods.end()) {
		methods[id].primitive.push_back(a);
	} else {
		AnonIE ie;
		if (len == -1) {
			if (!(ident = ipfix_id_lookup(id.id, id.enterprise))) {
				msg(MSG_ERROR, "Unknown or unsupported id %s detected.", id.toString().c_str());
				return;
			}
			len = ident->length;
		}
		ie.offset = ie.header = ie.packetClass = 0;
		// attention: if ie.len==0, anonField() assumes that it is a variable length field and the whole information element will be processed
		ie.len = len;
		ie.primitive.push_back(a);
		methods[id] = ie;
	}
}

/**
 * anonymises a field
 * @returns true when an anonymisation was performed
 */
bool AnonModule::anonField(InformationElement::IeInfo id, void* data, int len)
{
	if (methods.find(id) == methods.end()) {
		return false;
	}

	int l = methods[id].len;
	if (methods[id].len==0) {
		// this is a variable-length field, process everything
		l = len;
	}
	bool anonymized = false;
	for (std::vector<AnonPrimitive*>::iterator i = methods[id].primitive.begin(); i != methods[id].primitive.end(); ++i) {
        int cont = 1;
		(*i)->anonymizeBuffer(data, l, anonymized, &cont);
        if (! cont) break;
	}
	return anonymized;
}

