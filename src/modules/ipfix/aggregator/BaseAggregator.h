/*
 * Vermont Aggregator Subsystem
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

#ifndef BASEAGGREGATOR_H_
#define BASEAGGREGATOR_H_

#include "Rules.hpp"
#include "core/Module.h"
#include "common/Mutex.h"

#include <stdint.h>


class BaseAggregator 
	: public Module, public Source<IpfixRecord*>
{
public:
	BaseAggregator(uint32_t pollinterval);
	virtual ~BaseAggregator();
		
	void buildAggregator(Rules* rules, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);
	void buildAggregator(char* rulefile, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits);

	// events from Module
	virtual void preReconfiguration();
	virtual void onReconfiguration1();
	virtual void postReconfiguration();

	virtual string getStatisticsXML(double interval);

protected:
	Rules* rules; /**< Set of rules that define the aggregator */
	Mutex mutex; /**< ensures that exporterThread does not interfere with aggregation of incoming flows */
	
	/**
	 * creates a hashtable using the given parameters
	 * to be overloaded by specialised versions of aggregator module
	 */
	virtual BaseHashtable* createHashtable(Rule* rule, uint16_t minBufferTime, uint16_t maxBufferTime, uint8_t hashbits) = 0;
	void poll();
	void exporterThread();
	
	// events from Module
	virtual void performStart();
	virtual void performShutdown();
	
private:
	Thread thread;
	uint32_t pollInterval; /**< polling interval in milliseconds */
	
	static void* threadWrapper(void* instance);
};

#endif /*BASEAGGREGATOR_H_*/
