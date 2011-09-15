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

#ifndef BASEPLUGIN_H_
#define BASEPLUGIN_H_

#include "PluginState.h"
#include <string>
#include <string.h>
#include "modules/packet/Packet.h"
#include "HashtableBuckets.h"

using namespace std;

class BasePlugin{

public:
    bool operator== (const BasePlugin &other) const{
        if (pluginName.compare(other.pluginName) == 0 && pluginVersion.compare(other.pluginVersion) == 0){
            return true;
        }
        return false;
    }

    PluginState pluginState;
    string pluginName;
    string pluginVersion;
    virtual void newFlowReceived(const HashtableBucket* bucket) = 0;
    virtual void flowDeleted(const HashtableBucket* bucket) = 0;
    virtual void newPacketReceived(const Packet* p, uint32_t hash) = 0;
};

#endif
#endif
