/*
 * IPFIX Netflow Exporter
 * Copyright (C) 2011 Mario Volke
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

#ifndef IPFIXSIMPLETCPEXPORTER_H
#define IPFIXSIMPLETCPEXPORTER_H

#include "core/Module.h"
#include "core/Notifiable.h"
#include "IpfixRecordDestination.h"

#include <strings.h>

/**
 * IPFIX custom tcp exporter module
 *
 * Exports all received flows in a custom and simple format via TCP
 *
 */
class IpfixSimpleTcpExporter : public Module, public Source<NullEmitable*>, public IpfixRecordDestination
{
public:
	IpfixSimpleTcpExporter(string hostname, uint16_t port);
	virtual ~IpfixSimpleTcpExporter();

	// inherited from IpfixRecordDestination
	virtual void onDataRecord(IpfixDataRecord* record);

protected:

	void performStart();
	void performShutdown();

private:
	int sockfd;
	struct sockaddr_in siDest;
	uint16_t destPort;
};

#endif
