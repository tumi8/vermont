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

#ifndef OSFPPLUGIN_H_
#define OSFPPLUGIN_H_

#include "../BasePlugin.h"

// TCP Options structure
struct TCPOptions{
    bool has_options;
    bool sack_set;
    bool timestamp_set;
    bool mss_set;
    bool window_set;
    bool eol_set;
    bool unkown_option_set;
    bool unused;
    u_int32_t nop_set;
    u_int32_t tstamp;
    u_int32_t tsecr;
    u_int16_t window_scale;
    u_int16_t mss;
};

class OSFPPlugin : public BasePlugin{
public:
    OSFPPlugin();
    OSFPPlugin(u_int32_t maxConns, std::string file);
    void flowDeleted(const HashtableBucket* bucket);
    void newFlowReceived(const HashtableBucket* bucket);
    void newPacketReceived(const Packet* p);

private:
    bool writeHeaderFlag;
    u_int32_t maxConnections;
    std::string dumpFile;
    void processPacket(const Packet* p);
    int parseTCPOptions(struct TCPOptions &options, const Packet* p);
    void writeToFile(struct TCPOptions &options, const Packet* p);
};

template <class T>
std::string to_string(T t, std::ios_base & (*f)(std::ios_base&))
{
  std::ostringstream oss;
  oss << f << t;
  return oss.str();
}


#endif
