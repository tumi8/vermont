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
#ifndef BANNERGRABBINGPLUGIN_H_
#define BANNERGRABBINGPLUGIN_H_

#include "modules/ipfix/aggregator/BasePlugin.h"
#include "BannerOSMapping.h"
#include "OSResultAggregator.h"
#include <boost/unordered_map.hpp>
#include <iostream>
#include <fstream>
#include <map>

typedef boost::unordered_map<uint32_t, uint32_t> hashmap_t;
typedef pair<std::string, std::string> banner_pair_t;

class BannerGrabbingPlugin : public BasePlugin{
public:
    BannerGrabbingPlugin();
    BannerGrabbingPlugin(const uint32_t maxPckts, std::string file, std::string bannerfile);
    ~BannerGrabbingPlugin();
    void flowDeleted(const HashtableBucket* bucket);
    void newFlowReceived(const HashtableBucket* bucket);
    void newPacketReceived(const Packet* p, uint32_t hash);

    void initializeAggregator(uint32_t interval, std::string mode, std::string outputfile);

private:
    uint32_t maxPackets;
    ofstream filestream;
    std::string dumpFile;
    hashmap_t map;
    list<BannerOSMapping> banners;
    bool performOSGuessing;
    OSResultAggregator osAggregator;
    void processPacket(const Packet* p);
    void saveResult(const Packet* p, std::string* result_ptr);
    void guessOS(const Packet* p, std::string* grabbedString, BannerOSMapping::e_bannerType type);
    void processMsg(string message);
};

#endif
#endif
