/*
 * Vermont Packet Filter
 * Copyright (C) 2009  Vermont Project
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

#ifndef FILTERMODULE_H
#define FILTERMODULE_H

/**
	@author Peter Baumann <siprbaum@users.berlios.de>
*/


#include "core/Module.h"
#include "core/Destination.h"
#include "core/Source.h"
#include "modules/packet/filter/PacketProcessor.h"

class FilterModule
	: public Module, public Source<Packet*>, public Destination<Packet*>
{
public:
	FilterModule();
	virtual ~FilterModule();

	virtual void receive(Packet *);

	void addProcessor(PacketProcessor *p);
	std::vector<PacketProcessor*> getProcessors();

	bool hasReceiver();
	virtual std::string getStatisticsXML(double interval);

private:
	std::vector<PacketProcessor* > processors;
};

#endif
