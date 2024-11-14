/*
 * Vermont ZMQ receiver module
 * Copyright (c) 2015 by Brocade Communications Systems, Inc.
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

#ifdef ZMQ_SUPPORT_ENABLED

#include "IpfixReceiverZmq.hpp"

#include "IpfixPacketProcessor.hpp"
#include "IpfixParser.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/msg.h"

#include <stdexcept>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


using namespace std;

IpfixReceiverZmq::IpfixReceiverZmq(std::vector<std::string> endpoints,
		std::vector<std::string> channels, const int zmq_high_watermark,
		const int zmq_poll_timeout, unsigned int moduleId)
	: endpoints(endpoints), channels(channels),
	  zmq_high_watermark(zmq_high_watermark),
	  zmq_poll_timeout(zmq_poll_timeout),
	  statReceivedPackets(0)
{
	if (endpoints.empty()) {
		THROWEXCEPTION("No ZMQ channels configured, cannot start ZMQ Receiver");
	}

	// Stop CZMQ from hijacking signal handling
	zsys_handler_set(NULL);

	zpoller = zpoller_new(NULL);
	if (!zpoller) {
		THROWEXCEPTION("Could not create ZMQ poller, cannot start ZMQ Receiver");
	}

	for (const auto& endpoint : endpoints) {
		zsock_t *sock = zsock_new(ZMQ_SUB);
		if (!sock) {
			THROWEXCEPTION("Could not create ZMQ socket");
		}

		zsock_set_sndhwm(sock, zmq_high_watermark);
		zsock_set_rcvhwm(sock, zmq_high_watermark);

		if (zsock_connect(sock, "%s", (endpoint).c_str())) {
	                THROWEXCEPTION("Could not connect ZMQ socket");
	        }

		// If no channel is passed down, listen on everything (empty string)
		if (channels.empty()) {
			zsock_set_subscribe(sock, "");
		} else {
			for (const auto& channel : channels) {
				zsock_set_subscribe(sock, (channel).c_str());
			}
		}

		if (zpoller_add(zpoller, sock)) {
			THROWEXCEPTION("Could not add %s ZMQ socket to ZMQ poller", (endpoint).c_str());
		}

		zmq_sockets.push_back(sock);

		msg(LOG_NOTICE, "ZMQ Receiver listening on %s", (endpoint).c_str());
	}

	SensorManager::getInstance().addSensor(this, "IpfixReceiverZMQ", moduleId);
}

/**
 * Frees memory used by a IpfixReceiverZmq.
 */
IpfixReceiverZmq::~IpfixReceiverZmq()
{
	zpoller_destroy(&zpoller);

	for (auto& zmq_socket : zmq_sockets) {
		zsock_destroy(&zmq_socket);
	}

	msg(LOG_NOTICE, "Ipfix Receiver ZMQ poller and sockets destroyed");

	SensorManager::getInstance().removeSensor(this);
}

/**
 * ZMQ specific listener function. This function is called by @c listenerThread()
 */
void IpfixReceiverZmq::run()
{
	// create sourceId
	boost::shared_ptr<IpfixRecord::SourceID> sourceID(new IpfixRecord::SourceID);
	memset(sourceID->exporterAddress.ip, 0, 4);
	sourceID->exporterAddress.len = 4;
	sourceID->exporterPort = 0;
	sourceID->protocol = IPFIX_protocolIdentifier_RAW;
	sourceID->receiverPort = 0;
	sourceID->fileDescriptor = 0;

	while (!zsys_interrupted && !exitFlag) {
		DPRINTF_DEBUG("ZMQ Receiver: Waiting on Poller");
		void *sock = zpoller_wait(zpoller, zmq_poll_timeout);
		if (!sock) {
			if (zpoller_terminated(zpoller)) {
				msg(LOG_INFO, "ZMQ Receiver: ZMQ termination signal received");
				break;
			} else {
				DPRINTF_DEBUG("ZMQ Receiver: Poller Timeout");
				continue;
			}
		}

		zmsg_t *msg = zmsg_recv(sock);
		if (msg == NULL) {
			msg(LOG_ERR, "Empty ZMQ message");
			continue;
		}

		//Frame 0: extract the channel and throw it away, we don't care for now
		zframe_t *current_frame = zmsg_pop(msg);
		zframe_destroy(&current_frame);

		//Frame 1..n: flow reports
		size_t num_frames = zmsg_size(msg);
		for (size_t j = 0; j < num_frames; j++) {
			current_frame = zmsg_pop(msg);
			boost::shared_array<uint8_t> data(new uint8_t[zframe_size(current_frame)]);
			memcpy(data.get(), (uint8_t *)zframe_data(current_frame), zframe_size(current_frame));

			statReceivedPackets++;

			// send packet to all packet processors
			mutex.lock();
			for (auto& packetProcessor : packetProcessors) {
				(packetProcessor)->processPacket(data, zframe_size(current_frame), sourceID);
			}
			mutex.unlock();

			zframe_destroy(&current_frame);
		}
		zmsg_destroy(&msg);
	}
}

void IpfixReceiverZmq::clearStatistics()
{
    statReceivedPackets = 0;
}

/**
 * statistics function called by StatisticsManager
 */
std::string IpfixReceiverZmq::getStatisticsXML(double interval)
{
	ostringstream oss;

	oss << "<receivedPackets>" << statReceivedPackets << "</receivedPackets>" << endl;

	return oss.str();
}

#endif // ZMQ_SUPPORT_ENABLED
