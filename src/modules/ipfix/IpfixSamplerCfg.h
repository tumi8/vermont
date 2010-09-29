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

#ifndef IPFIXSAMPLERCFG_H_
#define IPFIXSAMPLERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/ipfix/IpfixSampler.h"

#include <string>

using namespace std;


class IpfixSamplerCfg
	: public CfgHelper<IpfixSampler, IpfixSamplerCfg>
{
public:
	friend class ConfigManager;

	virtual IpfixSamplerCfg* create(XMLElement* e);
	virtual ~IpfixSamplerCfg();

	virtual IpfixSampler* createInstance();
	virtual bool deriveFrom(IpfixSamplerCfg* old);

protected:

	double flowRate;

	IpfixSamplerCfg(XMLElement*);
};


#endif
