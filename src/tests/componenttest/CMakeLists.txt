#
# VERMONT build scripts for CMake
# Copyright (C) 2010 Simon Regnet <s-regnet@user.berlios.de>
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

ADD_EXECUTABLE(comptest
	ComponentTest.cpp
	QueueTest.cpp
)

TARGET_LINK_LIBRARIES(comptest
	modules
        ipfixlolib  
	core
        common        
        osdep      
	${CMAKE_THREAD_LIBS_INIT}
	${Boost_REGEX_LIBRARY}
	${Boost_FILESYSTEM_LIBRARY}
	${PCAP_LIBRARY}
	${LIBXML2_LIBRARIES}
)
