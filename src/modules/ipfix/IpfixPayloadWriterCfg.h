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

#ifndef IPFIXPAYLOADWRITERCFG_H_
#define IPFIXPAYLOADWRITERCFG_H_

#include <core/XMLElement.h>
#include <core/Cfg.h>

#include "modules/ipfix/IpfixPayloadWriter.h"

#include <string>

using namespace std;


class IpfixPayloadWriterCfg
	: public CfgHelper<IpfixPayloadWriter, IpfixPayloadWriterCfg>
{
public:
	friend class ConfigManager;

	virtual IpfixPayloadWriterCfg* create(XMLElement* e);
	virtual ~IpfixPayloadWriterCfg();

	virtual IpfixPayloadWriter* createInstance();
	virtual bool deriveFrom(IpfixPayloadWriterCfg* old);

protected:

	string path;			/**< path where files are written to */
	string filenamePrefix;	/**< prefix of all filenames that are created */
	uint32_t noConnections; /**< how many connections of a run should be recorded? */
	bool ignoreEmptyPayload; /**< ignores all flows having empty payload in both directions when set */
	bool ignoreIncompleteTCP; /**< ignores all TCP flows without set TCP flag in both directions */
	uint64_t startIdx; 		/**< start index of flows, important if dump needs to be continued */

	IpfixPayloadWriterCfg(XMLElement*);
};


#endif /*IPFIXPAYLOADWRITERCFG_H_*/
