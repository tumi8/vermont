/*
 * IPFIX Concentrator Module Library
 * Copyright (C) 2009 Tobias Limmer <http://www7.informatik.uni-erlangen.de/~limmer>
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

#ifndef _IPFIX_FILE_READER_H_
#define _IPFIX_FILE_READER_H_

#include <pthread.h>
#include <stdint.h>
#include <sys/types.h>
#include <list>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <boost/lexical_cast.hpp>

#include "IpfixReceiver.hpp"
#include "IpfixPacketProcessor.hpp"
#include <fstream>
/* Code adopted from Observer.cpp: */
/* subtract uvp from tvp and store in vvp */
#ifndef timersub
#define timersub(tvp, uvp, vvp)                 \
     do {                                        \
         (vvp)->tv_sec = (tvp)->tv_sec - (uvp)->tv_sec;      \
         (vvp)->tv_usec = (tvp)->tv_usec - (uvp)->tv_usec;   \
         if ((vvp)->tv_usec < 0) {               \
             (vvp)->tv_sec--;                    \
             (vvp)->tv_usec += 1000000;          \
         }                                       \
     } while (0)
#endif
/* compare tvp and uvp using cmp */
#ifndef timercmp
#define timercmp(tvp, uvp, cmp)                 \
    (((tvp)->tv_sec == (uvp)->tv_sec) ?         \
    ((tvp)->tv_usec cmp (uvp)->tv_usec) :       \
    ((tvp)->tv_sec cmp (uvp)->tv_sec))
#endif
/* multiply tvp by x and store in uvp */
#define timermul(tvp, uvp, x)                   \
    do {                                        \
        (uvp)->tv_sec = (tvp)->tv_sec * x;      \
        (uvp)->tv_usec = (tvp)->tv_usec * x;    \
        while((uvp)->tv_usec > 1000000) {       \
            (uvp)->tv_sec++;                    \
            (uvp)->tv_usec -= 1000000;          \
        }                                       \
    } while(0)
/* multiply tvp by x and store in uvp (with cast) */
#define timermulfloat(tvp, uvp, x)              \
    do {                                        \
        (uvp)->tv_sec = (time_t)((tvp)->tv_sec * x);      \
        (uvp)->tv_usec = (suseconds_t)((tvp)->tv_usec * x);    \
        while((uvp)->tv_usec > 1000000) {       \
            (uvp)->tv_sec++;                    \
            (uvp)->tv_usec -= 1000000;          \
        }                                       \
    } while(0)
#define settimezero(x)							\
	do {										\
		(x)->tv_sec = 0;						\
		(x)->tv_usec = 0;						\
	}											\
	while(0)

/**
 * reads raw packets from a directory containing one file per packet
 */
class IpfixReceiverFile : public IpfixReceiver {
public:
	IpfixReceiverFile(std::string, std::string, int, int, bool, float);
	virtual ~IpfixReceiverFile();

	virtual void run();
private:
	bool checkint(const char*);
	std::string packet_file_directory;
	std::string packet_file_basename;
	int from;
	int to;
	bool ignore_timestamps;
	float stretchTime;
	uint16_t stretchTimeInt;
	int fh;
	std::ifstream packetFile;
};

#endif
