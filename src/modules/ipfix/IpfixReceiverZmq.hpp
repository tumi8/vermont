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

#ifndef ZMQ_RECEIVER_HPP
#define ZMQ_RECEIVER_HPP

// default poll timeout, 1 second
#define ZMQ_POLL_TIMEOUT_DEFAULT 1000

#ifdef ZMQ_SUPPORT_ENABLED

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <czmq.h>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

class IpfixReceiverZmq : public IpfixReceiver, Sensor
{
public:
	IpfixReceiverZmq(std::vector<std::string> endpoints = std::vector<std::string>(),
			std::vector<std::string> channels = std::vector<std::string>(),
			int zmq_high_watermark = 0,
			int zmq_poll_timeout = ZMQ_POLL_TIMEOUT_DEFAULT);
	virtual ~IpfixReceiverZmq();

	virtual void run();
	virtual std::string getStatisticsXML(double interval);

protected:
	std::vector<std::string> endpoints;
	std::vector<std::string> channels;
	int zmq_high_watermark;
	int zmq_poll_timeout;

private:
	std::vector<zsock_t *> zmq_sockets;
	zpoller_t *zpoller;
	uint32_t statReceivedPackets;  /**< number of received packets */
};

#endif // ZMQ_SUPPORT_ENABLED

#endif // ZMQ_RECEIVER_HPP
