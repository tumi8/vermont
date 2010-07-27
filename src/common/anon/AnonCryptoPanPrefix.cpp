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

#include "AnonCryptoPanPrefix.h"

#include "common/msg.h"

#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <arpa/inet.h>

static int compare_cidrs(const void *d1, const void *d2){
    const int *da = (const int*) d1;
    const int *db = (const int*) d2;
    return (*da < *db) - (*da > *db);
}

    AnonCryptoPanPrefix::AnonCryptoPanPrefix (char* _key, std::vector<map_info> mapping)
: cryptopan ((const UINT8*)_key)
{
    number_of_cidrs=0;
    for(std::vector<map_info>::iterator it = mapping.begin(); it != mapping.end(); it++) 
    {
        struct in_addr inp;
        cpan_net_info info;
		info.fromNet = 0;
		info.toNet = 0;
		info.cidr = 0;
        int res = inet_aton((*it).fromNet.c_str(), &inp);
        if (!res) {
            THROWEXCEPTION("Invalid IP address: %s", (*it).fromNet.c_str());
        } else {
            info.fromNet = inp.s_addr;
        }

        res = inet_aton((*it).toNet.c_str(), &inp);
        if (!res) {
            THROWEXCEPTION("Invalid IP address: %s",(*it).toNet.c_str());
        } else {
            info.toNet = inp.s_addr;
        }

        try {   
            info.cidr = boost::lexical_cast<uint16_t>((*it).cidr.c_str());
        } catch (boost::bad_lexical_cast &) {
            THROWEXCEPTION("bad value for cidr: %s", (*it).cidr.c_str());
        }
        if (info.cidr > 32) {
            THROWEXCEPTION("Invalid cidr: %d", info.cidr);
        }
        bool addMe = true;
        for (int i=0; i<number_of_cidrs; i++) {
            if (avail_cidrs[i] == info.cidr) {
                addMe = false;
                break;
            }
        }
        if (addMe)
            avail_cidrs[number_of_cidrs++] = info.cidr;

        //info.fromNet = (info.fromNet) & ((1<<info.cidr)-1);
       // info.toNet = (info.toNet) & ((1<<info.cidr)-1);
        info.fromNet = ntohl(info.fromNet) & ~((1<<(32-info.cidr))-1);
        info.toNet = ntohl(info.toNet) & ~((1<<(32-info.cidr))-1);

        net_mapping[info.fromNet] = info;
    }
    qsort (&avail_cidrs[0], number_of_cidrs, sizeof (int), compare_cidrs);
}

AnonCryptoPanPrefix::~AnonCryptoPanPrefix ()
{
}

uint32_t AnonCryptoPanPrefix::pseudomize(uint32_t orig_addr) {
    std::map<uint32_t, cpan_net_info>::iterator it;
    uint16_t cidr;
    uint32_t a_addr, cs_id, tmp;
    
    for (int i = 0; i<number_of_cidrs; i++) {
        //tmp = orig_addr & ((1<<(avail_cidrs[i])) -1);
        tmp = ntohl(orig_addr) & ~((1<<(32-avail_cidrs[i])) -1);
        it = net_mapping.find(tmp);
        if (it == net_mapping.end())
            continue;

        //cs_id = ntohl((it->second).toNet); //csp->net_ctx[index].cs_net_id;
        cs_id = (it->second).toNet; //csp->net_ctx[index].cs_net_id;
        cidr = (it->second).cidr; //csp->net_ctx[index].cidr;
        a_addr = cryptopan.anonymize(ntohl(orig_addr));
        a_addr = ((a_addr << (cidr)) >> (cidr));
        cs_id = (cs_id >> (32 - cidr)) << (32 - cidr);
        return htonl(cs_id ^ a_addr);
    }
       return (orig_addr);

}



AnonPrimitive::ANON_RESULT AnonCryptoPanPrefix::anonymize(void* buf, unsigned int len)
{
    // IPv4 addresses are usually 4 bytes long, but Vermont internally handles 5 bytes with 1 byte ip mask
    assert ((len=sizeof(UINT32)) || (len == 5));
    UINT32 orig = 0;
    UINT32 cmp= 0;
    memcpy (&orig, buf, sizeof (UINT32));
    memcpy (&cmp, buf, sizeof (UINT32));
    orig = pseudomize(orig);
    memcpy (buf, &orig, sizeof (UINT32));
    if (cmp == orig)
        return ANON_RESULT (len, true);

    return ANON_RESULT (len, false);
}


