/*
 * Vermont Configuration Subsystem
 * Copyright (C) 2010 Vermont Project
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

#ifndef IPFIXCSEXPORTERCFG_H_
#define IPFIXCSEXPORTERCFG_H_
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "IpfixCsExporter.hpp"

#include <map>


class IpfixCsExporterCfg
	: public CfgHelper<IpfixCsExporter, IpfixCsExporterCfg>
{
public:
	friend class ConfigManager;

	virtual IpfixCsExporterCfg* create(XMLElement* e);

	virtual ~IpfixCsExporterCfg();

	virtual IpfixCsExporter* createInstance();

	virtual bool deriveFrom(IpfixCsExporterCfg* old);

protected:
	IpfixCsExporterCfg(XMLElement*);

private:
        //prefix to each file
        std::string filenamePrefix;
        //path where to store files
        std::string destinationPath;
        //maximum filesize in  KiB, i.e. maximumFilesize * 1024 == maximum filesize in bytes
        uint32_t maxFileSize;
        //maximum Chunk Buffer Time in seconds
        uint32_t maxChunkBufferTime;
        //maximum Chunk Buffer Records in records
        uint32_t maxChunkBufferRecords;
        //time in seconds between creation of a new output file
        uint32_t maxFileCreationInterval;
	uint8_t exportMode;
};

#endif /*IPFIXCSEXPORTERCFG_H_*/

