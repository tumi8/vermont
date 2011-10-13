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
#ifndef BANNEROSMAPPING_H_
#define BANNEROSMAPPING_H_

#include <string>
#include <list>
#include "core/XMLDocument.h"

class BannerOSMapping{

public:
    enum e_bannerType {
        HTTP=1,
        SSH
    };

    BannerOSMapping();
    BannerOSMapping(const std::string osType, const std::string osVersion, const std::string banner, const e_bannerType bannertype);
    std::string osType;
    std::string osVersion;
    std::string banner;
    e_bannerType bannertype;

    bool osTypeMatches(std::string other) const;
    bool osVersionMatches(std::string other) const;
    static std::list<BannerOSMapping> getBanners(std::string fileName);
    std::string findArchitecture(std::string other) const;
    double computeStringSimilarity(const std::string source, const std::string target) const;

private:
    static BannerOSMapping* readMapping(XMLElement* elem);
};

#endif
#endif
