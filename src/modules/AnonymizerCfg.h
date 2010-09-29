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

#ifndef _RECORD_ANONYMIZER_CFG_H_
#define _RECORD_ANONYMIZER_CFG_H_

#include <core/Cfg.h>

#include <modules/ipfix/IpfixRecordAnonymizer.h>

class AnonymizerCfg
	: public CfgHelper<IpfixRecordAnonymizer, AnonymizerCfg>
{
public:
	friend class ConfigManager;

	virtual AnonymizerCfg* create(XMLElement* e);

	virtual ~AnonymizerCfg();

	virtual IpfixRecordAnonymizer* createInstance();

	virtual bool deriveFrom(AnonymizerCfg* old);

	static void initInstance(CfgBase* c, AnonModule* module, XMLNode::XMLSet<XMLElement*> set);

protected:
	AnonymizerCfg(XMLElement*);
};

#endif
