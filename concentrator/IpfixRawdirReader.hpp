/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2004 Christoph Sommer <http://www.deltadevelopment.de/users/christoph/ipfix/>
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

#ifdef HAVE_BOOST_FILESYSTEM

#ifndef _IPFIX_RAWDIR_READER_H_
#define _IPFIX_RAWDIR_READER_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <list>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "boost/progress.hpp"

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"

/**
 * reads raw packets from a directory containing one file per packet
 */
class IpfixRawdirReader : public IpfixReceiver {
public:
	IpfixRawdirReader(std::string packet_directory_path);
	virtual ~IpfixRawdirReader();

	virtual void run();
private:
	std::string packet_directory_path;
	boost::filesystem::directory_iterator dir_iterator;
	boost::filesystem::directory_iterator end_iterator;
};

#endif

#endif //HAVE_BOOST_FILESYSTEM
