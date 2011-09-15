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
#ifndef OSFPPLUGIN_H_
#define OSFPPLUGIN_H_

#include "modules/ipfix/aggregator/BasePlugin.h"
#include <boost/unordered_map.hpp>
#include <iostream>
#include <fstream>

// TCP Options structure
struct TCPOptions{
    bool has_options;
    bool sack_set;
    bool timestamp_set;
    bool mss_set;
    bool window_set;
    bool eol_set;
    bool unkown_option_set;
    bool options_corrupt;
    u_int32_t nop_set;
    u_int32_t tstamp;
    u_int32_t tsecr;
    u_int8_t window_scale;
    u_int16_t mss;
};

typedef boost::unordered_map<u_int32_t, u_int32_t> hashmap_t;

class OSFPPlugin : public BasePlugin{
public:
    OSFPPlugin();
    OSFPPlugin(const u_int32_t maxPckts, std::string file);
    ~OSFPPlugin();
    void flowDeleted(const HashtableBucket* bucket);
    void newFlowReceived(const HashtableBucket* bucket);
    void newPacketReceived(const Packet* p, uint32_t hash);

private:
    u_int32_t maxPackets;
    std::string dumpFile;
    hashmap_t map;
    void processPacket(const Packet* p);
    int parseTCPOptions(struct TCPOptions &options, const Packet* p, const u_int32_t dataOffset);
    void writeToFile(struct TCPOptions &options, const Packet* p);
    ofstream myfile;
};

#endif
#endif
