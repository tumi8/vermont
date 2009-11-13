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

#include <sys/time.h>
#include <time.h>

#include "common/msg.h"
#include "common/Misc.h"
#include "common/ipfixlolib/ipfixlolib.h"

#include "FastPSAMPExporterModule.h"
#include "FastPSAMPSender.h"
#include <signal.h>

using namespace std;

FastPSAMPExporterModule::FastPSAMPExporterModule(const std::string& ip,
		uint16_t port) :
	sender(NULL) {
	/*ignore sigpipe, better way for this?*/
	signal(SIGPIPE, SIG_IGN);
	sender = new FastPSAMPSender::FastPSAMPSender(ip, port, true);
	sender->setsendTemplateeverySecs(15);
}

FastPSAMPExporterModule::~FastPSAMPExporterModule() {
	if (sender != NULL)
		delete sender;
}

void FastPSAMPExporterModule::receive(Packet* p) {
	sender->handlePacket(p);
	send(p);
}

string FastPSAMPExporterModule::getStatisticsXML(double interval) {
	if (sender == NULL)
		return "";
	return sender->getStatisticsXML(interval);
}
