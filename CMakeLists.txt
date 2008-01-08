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

PROJECT(VERMONT)


### CMake configuration

# allow building with old CMake. Use some bundled modules as a fallback
CMAKE_MINIMUM_REQUIRED(VERSION 2.3.5)
SET(CMAKE_MODULE_PATH ${CMAKE_ROOT}/Modules ${CMAKE_SOURCE_DIR}/cmake/modules)

# move some config clutter to the advanced section
MARK_AS_ADVANCED(
	CMAKE_BACKWARDS_COMPATIBILITY
	CMAKE_BUILD_TYPE
	CMAKE_INSTALL_PREFIX
	EXECUTABLE_OUTPUT_PATH
	LIBRARY_OUTPUT_PATH
)


### basic modules

SUBDIRS(
	osdep
	cfg
	common
	concentrator
	ipfixlolib
	sampler
	reconf
)

ADD_EXECUTABLE(vermont
	vermont.cc
)

INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/cfg)
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/common)
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/concentrator)
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/ipfixlolib)
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/reconf)
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/sampler)
INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/osdep/linux)

TARGET_LINK_LIBRARIES(vermont
	cfg
	reconf
	ipfixlolib
	sampler
	common
	osdep
	concentrator	
)

INSTALL(TARGETS vermont
	RUNTIME DESTINATION bin
)

INSTALL(FILES README CONFIGURATION LICENSE
	DESTINATION share/doc/vermont
)

INSTALL(FILES ipfix-config-schema.xsd
	DESTINATION share/vermont
)

INSTALL(DIRECTORY configs/
	DESTINATION share/vermont/configs
	PATTERN ".svn" EXCLUDE
)


### doxygen

FIND_PACKAGE(Doxygen REQUIRED)


### threads

FIND_PACKAGE(Threads REQUIRED)
TARGET_LINK_LIBRARIES(vermont
	${CMAKE_THREAD_LIBS_INIT}
)

### boost

FIND_PACKAGE(Boost REQUIRED)
MARK_AS_ADVANCED(
	Boost_INCLUDE_DIR
	Boost_REGEX_LIBRARY
	Boost_FILESYSTEM_LIBRARY
	Boost_UNIT_TEST_FRAMEWORK_LIBRARY
)
IF (Boost_FOUND)
	MESSAGE(STATUS "Found boost libraries")
	ADD_DEFINITIONS(-DHAVE_BOOST_FILESYSTEM)
	INCLUDE_DIRECTORIES(${Boost_INCLUDE_DIRS})
	FIND_LIBRARY(Boost_REGEX_LIBRARY NAMES boost_regex-mt boost_regex PATHS ${Boost_LIBRARY_DIRS})
	FIND_LIBRARY(Boost_FILESYSTEM_LIBRARY NAMES boost_filesystem-mt boost_filesystem PATHS ${Boost_LIBRARY_DIRS})
	FIND_LIBRARY(Boost_UNIT_TEST_FRAMEWORK_LIBRARY NAMES boost_unit_test_framework-mt boost_unit_test_framework PATHS ${Boost_LIBRARY_DIRS})
	TARGET_LINK_LIBRARIES(vermont
		${Boost_REGEX_LIBRARY}
		${Boost_FILESYSTEM_LIBRARY}
	)
ELSE (Boost_FOUND)
	MESSAGE(FATAL_ERROR "Could not find boost libraries")
	REMOVE_DEFINITIONS(-DHAVE_BOOST_FILESYSTEM)
ENDIF (Boost_FOUND)


### libxml2

FIND_PACKAGE(LibXml2 REQUIRED)
MARK_AS_ADVANCED(
	LIBXML2_INCLUDE_DIR
	LIBXML2_LIBRARIES
)
IF (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
	MESSAGE(STATUS "Found libxml2 libraries")
	INCLUDE_DIRECTORIES(${LIBXML2_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${LIBXML2_LIBRARIES}
	)
ELSE (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
	MESSAGE(FATAL_ERROR "Could not find libxml2 libraries")
ENDIF (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)


### debug

OPTION(DEBUG "Enable debug code. Vermont will run significantly slower if enabled." OFF)
IF (DEBUG)
	message(STATUS "Configuring build process for debug version")
	REMOVE_DEFINITIONS(-O3)
	ADD_DEFINITIONS(-O0 -g -pg -Wall -Werror -DDEBUG)
ELSE (DEBUG)
	REMOVE_DEFINITIONS(-O0 -g -pg -Wall -Werror -DDEBUG)
	ADD_DEFINITIONS(-O3)
ENDIF (DEBUG)


### IP_HEADER_OFFSET

SET(IP_HEADER_OFFSET 14 CACHE STRING "Start position of the IP header in an ethernet frame in Bytes. This value needs to be adjusted according to the network monitored. The default value is 14 for ethernet devices. Other common values are 4 (BSD loop back device) and 18 (Ethernet VLAN)")
ADD_DEFINITIONS(-DIP_HEADER_OFFSET=${IP_HEADER_OFFSET})


### PCAP_MAX_CAPTURE_LENGTH

SET(PCAP_MAX_CAPTURE_LENGTH 128 CACHE STRING "Maximum PCAP packet capture length (this amount of bytes is always allocated for each packet, the smaller the better!)")
ADD_DEFINITIONS(-DPCAP_MAX_CAPTURE_LENGTH=${PCAP_MAX_CAPTURE_LENGTH})


### SUPPORT_NETFLOWV9

OPTION(SUPPORT_NETFLOWV9 "Enable NetFlow version 9 support" ON)
IF (SUPPORT_NETFLOWV9)
	ADD_DEFINITIONS(-DSUPPORT_NETFLOWV9)
ELSE (SUPPORT_NETFLOWV9)
	REMOVE_DEFINITIONS(-DSUPPORT_NETFLOWV9)
ENDIF (SUPPORT_NETFLOWV9)


### MySQL

OPTION(SUPPORT_MYSQL "Enable dbwriter/dbreader support" ON)
IF (SUPPORT_MYSQL)
	FIND_PACKAGE(MySQL REQUIRED)
	MARK_AS_ADVANCED(
		MYSQL_ADD_INCLUDE_DIR
		MYSQL_ADD_LIBRARY
		MYSQL_CONFIG
		MYSQL_CONFIG_PREFER_PATH
	)
	IF (NOT MYSQL_FOUND)
		MESSAGE(STATUS "Could not find MySQL libraries. Disabling dbwriter/dbreader support.")
	ENDIF (NOT MYSQL_FOUND)
ENDIF (SUPPORT_MYSQL)
IF (MYSQL_FOUND)
	MESSAGE(STATUS "Found MySQL libraries")
	ADD_DEFINITIONS(-DDB_SUPPORT_ENABLED)
	INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${MYSQL_LIBRARIES}
	)
ELSE (MYSQL_FOUND)
	REMOVE_DEFINITIONS(-DDB_SUPPORT_ENABLED)
ENDIF (MYSQL_FOUND)


### libpcap-mmap

OPTION(USE_PCAPMMAP "Use libpcap-mmap." OFF)
MARK_AS_ADVANCED(
	PCAP_LIBRARY_REGULAR
	PCAP_LIBRARY_MMAP
)

IF (USE_PCAPMMAP)
    FIND_LIBRARY(PCAP_LIBRARY_MMAP NAMES pcap pcap-mmap PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/../libpcap-mmap ${VERMONT_SOURCE_DIR}/../../../trunk/libpcap-mmap NO_DEFAULT_PATH)
    SET(PCAP_LIBRARY "${PCAP_LIBRARY_MMAP}")
    IF (PCAP_LIBRARY_MMAP)
        SET(PCAP_LIBRARY "${PCAP_LIBRARY_MMAP}")
    ELSE (PCAP_LIBRARY_MMAP)
        MESSAGE(FATAL_ERROR "Could not find libpcap-mmap")
    ENDIF (PCAP_LIBRARY_MMAP)
ELSE (USE_PCAPMMAP)
    FIND_LIBRARY(PCAP_LIBRARY_REGULAR NAMES pcap)
    IF (PCAP_LIBRARY_REGULAR)
        SET(PCAP_LIBRARY "${PCAP_LIBRARY_REGULAR}")
    ELSE (PCAP_LIBRARY_REGULAR)
        MESSAGE(FATAL_ERROR "Could not find libpcap")
    ENDIF (PCAP_LIBRARY_REGULAR)
ENDIF (USE_PCAPMMAP)
TARGET_LINK_LIBRARIES(vermont ${PCAP_LIBRARY})


### tools

OPTION(WITH_TOOLS "Build misc tools." ON)
IF (WITH_TOOLS)
	SUBDIRS(tools)
ELSE (WITH_TOOLS)
ENDIF (WITH_TOOLS)


### tests

OPTION(WITH_TESTS "Build unit tests." ON)
IF (WITH_TESTS)
	SUBDIRS(tests)
ELSE (WITH_TESTS)
ENDIF (WITH_TESTS)


