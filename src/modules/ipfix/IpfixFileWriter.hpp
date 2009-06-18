/*
 * IPFIX Database Reader/Writer
 * Copyright (C) 2006 Jürgen Abberger
 * Copyright (C) 2006 Lothar Braun <braunl@informatik.uni-tuebingen.de>
 * Copyright (C) 2007 Gerhard Muenz
 * Copyright (C) 2008 Tobias Limmer
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


#ifndef IPFIXFILEWRITER_H
#define IPFIXFILEWRITER_H

#include "IpfixParser.hpp"
#include "IpfixRecord.hpp"
#include "common/ipfixlolib/ipfix.h"
#include "common/ipfixlolib/ipfixlolib.h"
#include "modules/ipfix/IpfixSender.hpp"
#include <netinet/in.h>
#include <time.h>
#include <iostream>
#include <fstream>

#define EXPORTERID 0
#define DEFAULTFILESIZE 2147483647

class IpfixFileWriter :  public IpfixSender
{
	public:
		std::string filenamePrefix;
		std::string destinationPath;
		uint64_t maximumFilesize;
		IpfixFileWriter(uint16_t observationDomainId, std::string filenamePrefix, 
			std::string destinationPath, uint64_t maximumFilesize);

		~IpfixFileWriter();
		int addCollector(uint16_t observationDomainId, std::string filenamePrefix, 
					std::string destinationPath, uint64_t maximumFilesize);


	private:
		std::string filename;
		uint32_t filenum;
		uint64_t byteswritten;

};


#endif
