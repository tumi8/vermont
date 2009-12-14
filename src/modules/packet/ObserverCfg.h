/*
 * Vermont Configuration Subsystem
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

#ifndef OBSERVERCFG_H_
#define OBSERVERCFG_H_

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include <modules/packet//Observer.h>

#include <core/InstanceManager.h>
#include <map>

class Observer;

class ObserverCfg
	: public CfgHelper<Observer, ObserverCfg>
{
public:
	friend class ConfigManager;

	virtual ObserverCfg* create(XMLElement* e);

	virtual ~ObserverCfg();

	virtual Observer* createInstance();

	virtual bool deriveFrom(ObserverCfg* old);

protected:
	ObserverCfg(XMLElement*);

private:
	// config variables
	std::string interface;	// also used for filename in offline mode
	std::string pcap_filter;
	unsigned int capture_len;
	bool offline;
	bool replaceOfflineTimestamps;
	bool offlineAutoExit;
	float offlineSpeed;
	uint64_t maxPackets;
};

#endif /*OBSERVERCFG_H_*/
