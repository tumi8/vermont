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
#ifndef BASEPLUGINHOST_H_
#define BASEPLUGINHOST_H_

#include "BasePlugin.h"
#include <list>
using namespace std;

class BasePluginHost
{
public:
    static BasePluginHost* getInstance();
    static void release();
    list<BasePlugin*> getPlugins();
    int registerPlugin(BasePlugin* plugin);
    int unregisterPlugin(BasePlugin* plugin);

private:
    BasePluginHost(void) { msg(MSG_INFO, "PluginHost instantiated"); }
    ~BasePluginHost(void) { msg(MSG_INFO, "PluginHost deleted"); }
    static BasePluginHost *instance;
    list<BasePlugin*> plugins;
};

#endif
#endif
