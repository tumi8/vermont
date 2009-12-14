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

#ifndef FLOWMETERINGCFG_H_
#define FLOWMETERINGCFG_H_

#include "AggregatorBaseCfg.h"
#include "core/Cfg.h"
#include "modules/ipfix/aggregator/IpfixAggregator.hpp"

class IpfixAggregatorCfg
	: public CfgHelper<IpfixAggregator, IpfixAggregatorCfg>, public AggregatorBaseCfg
{
public:
	friend class ConfigManager;
	
	virtual ~IpfixAggregatorCfg();

	virtual IpfixAggregatorCfg* create(XMLElement* elem);
	
	virtual IpfixAggregator* createInstance();
	
	bool deriveFrom(IpfixAggregatorCfg* old);


protected:
	IpfixAggregatorCfg(XMLElement* elem);
	
};

#endif /*FLOWMETERINGCFG_H_*/
