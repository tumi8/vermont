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

#ifndef PYTHONFLOWANALYZERCFG_H_
#define PYTHONFLOWANALYZERCFG_H_

#include "core/InfoElementCfg.h"
#include "PythonFlowAnalyzer.h"

#include <string>


class PythonFlowAnalyzerCfg : public CfgHelper<PythonFlowAnalyzer, PythonFlowAnalyzerCfg>
{
public:
	friend class ConfigManager;

	std::string getName() { return "pythonFlowAnalyzer"; }

	PythonFlowAnalyzer* createInstance();
	virtual PythonFlowAnalyzerCfg* create(XMLElement* e);
	virtual bool deriveFrom(PythonFlowAnalyzerCfg* old);

protected:
	PythonFlowAnalyzerCfg(XMLElement*);

private:
	string code;
};

#endif /* PYTHONFLOWANALYZERCFG_H_ */
