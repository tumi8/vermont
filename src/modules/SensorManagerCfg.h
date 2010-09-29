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

#ifndef SENSORMANAGERCFG_H_
#define SENSORMANAGERCFG_H_

#include "core/SensorManager.h"

#include "core/XMLElement.h"
#include "core//Cfg.h"

#include <string>

using namespace std;


class SensorManagerCfg
	: public CfgHelper<SensorManager, SensorManagerCfg>
{
public:
	friend class ConfigManager;
	
	virtual ~SensorManagerCfg();
	
	virtual SensorManagerCfg* create(XMLElement* e);
	virtual SensorManager* createInstance();
	virtual bool deriveFrom(SensorManagerCfg* old);
	
	void setGraphIS(GraphInstanceSupplier* gis);
	
protected:
	SensorManagerCfg(XMLElement*);
	
private:
	GraphInstanceSupplier* graphIS;
	static bool instanceCreated;
	
	// config variables
	uint32_t checkInterval; /** sensor check interval in seconds */
	string sensorOutput; /** filename of output file which contains sensor data */
	bool append;	/** append to output file or overwrite */
};

#endif /*SENSORMANAGERCFG_H_*/
