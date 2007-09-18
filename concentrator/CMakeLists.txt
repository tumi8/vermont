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

ADD_LIBRARY(concentrator
	IpfixAggregator.cpp
	ipfix.cpp
	IpfixReceiver.cpp
	IpfixReceiverUdpIpV4.cpp
	IpfixRawdirReader.cpp
	FlowSource.cpp
	FlowSink.cpp
	IpfixRecord.cpp
	IpfixPrinter.cpp
	IpfixParser.cpp
	IpfixPacketProcessor.cpp
	IpfixCollector.cpp
	Rules.cpp
	Rule.cpp
	IpfixSender.cpp
	IpfixRawdirWriter.cpp
	TemplateBuffer.cpp
	IpfixDbReader.cpp
	IpfixDbWriter.cpp
)

