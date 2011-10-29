/*
 * NetflowV9 to IPFIX Converter Module
 * Copyright (C) 2009 Gerhard Muenz
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

#ifndef _NETFLOWV9CONVERTERCFG_H_
#define _NETFLOWV9CONVERTERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/ipfix/NetflowV9Converter.hpp"

#include <string>

using namespace std;


class NetflowV9ConverterCfg
	: public CfgHelper<NetflowV9Converter, NetflowV9ConverterCfg>
{
public:
	friend class ConfigManager;

	virtual NetflowV9ConverterCfg* create(XMLElement* e);
	virtual ~NetflowV9ConverterCfg();

	virtual NetflowV9Converter* createInstance();
	virtual bool deriveFrom(NetflowV9ConverterCfg* old);

protected:

	bool copyMode;
	bool keepFlowSysUpTime;

	NetflowV9ConverterCfg(XMLElement*);
};


#endif
