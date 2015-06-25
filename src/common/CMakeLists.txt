#
# VERMONT build scripts for CMake
# Copyright (C) 2007 Christoph Sommer <christoph.sommer@informatik.uni-erlangen.de>
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#

ADD_LIBRARY(common
	CountingSemaphore.cpp
	SignalHandler.cpp
	SignalInterface.h
	msg.cc
	Sensor.cpp
	VermontControl.cpp
	Misc.cpp
	bloom/BloomFilter.cpp
	bloom/AgeBloomFilter.cpp
	bloom/CountBloomFilter.cpp
	cryptopan/panonymizer.cpp
	cryptopan/rijndael.cpp
	hmacsha1/sha1.cpp
	hmacsha1/sha1_hmac.cpp
	openssl/OpenSSL.cpp
	openssl/SSLCTXWrapper.cpp
)

add_cppcheck(common STYLE POSSIBLE_ERROR)

SUBDIRS(
	anon
	ipfixlolib	
)

