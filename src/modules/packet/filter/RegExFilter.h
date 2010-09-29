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

/** @file
 * Filter a packet by checking if it is containing a predefined regEx string
 */

#ifndef REGEXFILTER_H
#define REGEXFILTER_H

#include <list>
#include <string>
#include <string.h>
#include "common/msg.h"
#include "PacketProcessor.h"
#include <sys/types.h>
#include <boost/regex.hpp>



class RegExFilter
	: public PacketProcessor
{

public:

  RegExFilter ()
  {
  };

  virtual ~RegExFilter ()
  {
  };

  void regcre()
  {
  	rexp.assign(match);
  };

  virtual bool processPacket (Packet * p);

  int filtertype;
  std::string match;
  boost::regex rexp;

protected:
  bool compare (char *data);

};

#endif
