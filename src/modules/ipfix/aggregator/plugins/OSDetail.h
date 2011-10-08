/*
 * Vermont Aggregator Subsystem
 * Copyright (C) 2009 Vermont Project
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

#ifdef PLUGIN_SUPPORT_ENABLED
#ifndef OSDETAIL_H_
#define OSDETAIL_H_
#include <boost/functional/hash.hpp>

enum e_origin{FINGERPRINT, BANNER};

class OSDetail{
    friend std::size_t hash_value(OSDetail const& detail){
        std::size_t seed = 0;
        boost::hash_combine(seed, detail.os_type);
        boost::hash_combine(seed, detail.os_version);
        boost::hash_combine(seed, detail.architecture);
        boost::hash_combine(seed, detail.origin);

        return seed;
    }

public:
    OSDetail();
    OSDetail(std::string os_type, std::string os_version, std::string architecture, e_origin origin);
    bool operator==(OSDetail const& other) const{
        return os_type == other.os_type && os_version == other.os_version && architecture == other.architecture && origin == other.origin;
    }
    std::string os_type;
    std::string os_version;
    std::string architecture;
    e_origin origin;
};

#endif
#endif
