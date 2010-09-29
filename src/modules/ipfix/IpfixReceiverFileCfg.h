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

#ifndef IPFIXRECEIVERFILECFG_H_
#define IPFIXRECEIVERFILECFG_H_

#include "core/Cfg.h"
#include <modules/ipfix/IpfixCollector.hpp>
#include <modules/ipfix/IpfixReceiverFile.hpp>

class IpfixReceiverFileCfg
	: public CfgHelper<IpfixCollector, IpfixReceiverFileCfg>
{
public:
	IpfixReceiverFileCfg(XMLElement* elem);
	virtual ~IpfixReceiverFileCfg();
	
	virtual IpfixReceiverFileCfg* create(XMLElement* elem);
	
	virtual IpfixCollector* createInstance();
	
	bool deriveFrom(IpfixReceiverFileCfg* old);
	
private:
        IpfixCollector* ipfixCollector;
        uint16_t observationDomainId;
		std::string packetFileBasename;
		std::string packetFileDirectory;
		int c_from;
		int c_to;
		bool ignore;
		float offlinespeed;
};

#endif /*IPFIXRECEIVERFILECFG_H_*/
