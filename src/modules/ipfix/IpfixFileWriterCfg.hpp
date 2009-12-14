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

#ifndef IPFIXFILEWRITERCFG_H_
#define IPFIXFILEWRITERCFG_H_
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixFileWriter.hpp"

#include <map>


class IpfixFileWriterCfg
	: public CfgHelper<IpfixFileWriter, IpfixFileWriterCfg>
{
public:
	friend class ConfigManager;

	virtual IpfixFileWriterCfg* create(XMLElement* e);

	virtual ~IpfixFileWriterCfg();

	virtual IpfixFileWriter* createInstance();

	virtual bool deriveFrom(IpfixFileWriterCfg* old);

protected:
	IpfixFileWriterCfg(XMLElement*);

private:
	// config variables
	std::string destinationPath;
	std::string filenamePrefix;
	uint32_t maximumFilesize;
	uint16_t observationDomainId;
};

#endif /*IPFIXFILEWRITERCFG_H_*/

