/*
  VERMONT 
 * Copyright (C) 2012 Lothar Braun <braun@net.in.tum.de>
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
 */

#ifndef _FLOWLENALYZER_CFG_H_
#define _FLOWLENALYZER_CFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "FlowLenAnalyzer.h"

class FlowLenAnalyzerCfg
	: public CfgHelper<FlowLenAnalyzer, FlowLenAnalyzerCfg>
{
public:
	friend class ConfigManager;
	
	virtual FlowLenAnalyzerCfg* create(XMLElement* e);
	virtual ~FlowLenAnalyzerCfg();
	
	virtual FlowLenAnalyzer* createInstance();
	virtual bool deriveFrom(FlowLenAnalyzerCfg* old);
	
protected:
	std::string flowFilename;
	std::string binFilename;
	std::vector<uint64_t> bins;

	FlowLenAnalyzerCfg(XMLElement*);
};


#endif 
