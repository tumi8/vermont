/*
 * VERMONT
 * Copyright (C) 2009 Matthias Segschneider <matthias.segschneider@gmx.de>
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

#include "PythonFlowAnalyzerCfg.h"

#include <arpa/inet.h>


PythonFlowAnalyzerCfg::PythonFlowAnalyzerCfg(XMLElement* elem)
: CfgHelper<PythonFlowAnalyzer, PythonFlowAnalyzerCfg>(elem, "pythonFlowAnalyzer")
{
	if (!elem) return;  // needed because of table inside ConfigManager

	try {
		code = get("code");
	} catch(IllegalEntry ie) {
		THROWEXCEPTION("Illegal hostStatistics entry in config file");
	}
}

bool PythonFlowAnalyzerCfg::deriveFrom(PythonFlowAnalyzerCfg* old)
{
	return false;
}

PythonFlowAnalyzerCfg* PythonFlowAnalyzerCfg::create(XMLElement* e)
{
	assert(e);
	assert(e->getName() == getName());
	return new PythonFlowAnalyzerCfg(e);
}

PythonFlowAnalyzer* PythonFlowAnalyzerCfg::createInstance()
{
	if (!instance) {
		instance = new PythonFlowAnalyzer(code);
	}
	return instance;
}
