/*
 * VERMONT
 * Copyright (C) 2009 Daniel Wilhelm <e-mail@jiaz.de>
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

#ifndef FastEXPORTER_SINK_H
#define FastEXPORTER_SINK_H

#include "common/ConcurrentQueue.h"
#include "common/msg.h"

#include "common/Thread.h"
#include "modules/packet/Template.h"
#include "modules/packet/Packet.h"

#include "core/Module.h"

#include "modules/packet/FastPSAMPSender.h"

class Packet;

class FastPSAMPExporterModule: public Module,
		public Source<Packet*> ,
		public Destination<Packet*> {
public:
	FastPSAMPExporterModule(const std::string& ip, uint16_t port);
	~FastPSAMPExporterModule();

	virtual void receive(Packet* packet);
	virtual string getStatisticsXML(double interval);

private:
	FastPSAMPSender *sender;
};

#endif
