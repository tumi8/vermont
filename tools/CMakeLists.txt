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

ADD_EXECUTABLE(testCollector
	testCollector.cpp
)

TARGET_LINK_LIBRARIES(testCollector	
	modules
	core
	ipfixlolib
	common
	osdep
	Boost::regex
	Boost::filesystem
	Boost::system
	${CMAKE_THREAD_LIBS_INIT}
	${PCAP_LIBRARY}
)

IF (SUPPORT_DTLS)
	TARGET_LINK_LIBRARIES(testCollector ${OPENSSL_LIBRARIES})
	IF (CRYPTO_FOUND)
		TARGET_LINK_LIBRARIES(testCollector ${CRYPTO_LIBRARIES})
	ENDIF (CRYPTO_FOUND)
ENDIF (SUPPORT_DTLS)

IF (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
	MESSAGE(STATUS "Found libxml2 libraries")
	INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(testCollector
		${LIBXML2_LIBRARIES}
	)
ENDIF (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)


ADD_EXECUTABLE(injectUDPToCollector
	injectUDPToCollector.cpp
)

TARGET_LINK_LIBRARIES(injectUDPToCollector
	${PCAP_LIBRARY}
)

IF (JOURNALD_FOUND)
	TARGET_LINK_LIBRARIES(testCollector
		${JOURNALD_LIBRARIES}
	)
ENDIF (JOURNALD_FOUND)
