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
#include "BasePluginHost.h"

BasePluginHost* BasePluginHost::instance = NULL;

BasePluginHost* BasePluginHost::getInstance()
{
    if( instance == NULL )
        instance = new BasePluginHost();
    return instance;
}

void BasePluginHost::release(){
    if( instance != NULL )
        delete instance;
    instance = NULL;
}

/*
 * get all registered plugin
 */
list<BasePlugin*> BasePluginHost::getPlugins(){
    return plugins;
}

/*
 * register a new plugin
 */
int BasePluginHost::registerPlugin(BasePlugin* plugin){
    plugins.insert(plugins.end(), plugin);
    return 1;
}

/*
 * unregister a plugin
 */
int BasePluginHost::unregisterPlugin(BasePlugin* plugin){
    list<BasePlugin*>::iterator it;
    for (it = plugins.begin(); it != plugins.end(); it++) {
        if((BasePlugin*)*it == plugin){
            plugins.erase(it);
            return 1;
        }
    }
    return 0;
}

#endif
