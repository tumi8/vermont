//
// Copyright (C) 2008 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

#ifndef __ANON_CRYPTO_PAN_PREFIX_H
#define __ANON_CRYPTO_PAN_PREFIX_H

#include <string>
#include <cassert>
#include <map>
#include <vector>
#include <common/cryptopan/panonymizer.h>
#include "AnonPrimitive.h"
#include <common/anon/CrpytoPanInfoElements.h>

#include "common/msg.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <boost/lexical_cast.hpp>




class AnonCryptoPanPrefix : public AnonPrimitive {
public:
	AnonCryptoPanPrefix(char*  _key,std::vector<map_info> mapping);
	~AnonCryptoPanPrefix();
    uint32_t pseudomize(uint32_t orig_addr);

protected:

	ANON_RESULT anonymize(void* buf, unsigned int len);

private:

	PAnonymizer cryptopan;
    std::map<uint32_t, cpan_net_info> net_mapping;
    int avail_cidrs[32];
    int number_of_cidrs;

};

#endif // __ANON_CRYPTO_PAN_H

