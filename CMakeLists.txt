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
	src
)

if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)
    cmake_policy(SET CMP0002 NEW)
endif(COMMAND cmake_policy)


ADD_EXECUTABLE(vermont
	src/vermont.cc
)

INCLUDE_DIRECTORIES(${VERMONT_SOURCE_DIR}/src)

TARGET_LINK_LIBRARIES(vermont
	modules
	core
	anon
	ipfixlolib
	common
	osdep
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

#FIND_PACKAGE(Doxygen REQUIRED)


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
	IF (NOT Boost_REGEX_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost regex library")
	ENDIF(NOT Boost_REGEX_LIBRARY)
	FIND_LIBRARY(Boost_FILESYSTEM_LIBRARY NAMES boost_filesystem-mt boost_filesystem PATHS ${Boost_LIBRARY_DIRS})
	IF (NOT Boost_FILESYSTEM_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost filesystem library")
	ENDIF(NOT Boost_FILESYSTEM_LIBRARY)
	FIND_LIBRARY(Boost_UNIT_TEST_FRAMEWORK_LIBRARY NAMES boost_unit_test_framework-mt boost_unit_test_framework PATHS ${Boost_LIBRARY_DIRS})
	IF (NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost unit test framework")
	ENDIF (NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
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
	ADD_DEFINITIONS(-O0 -g -pg -Wall -DDEBUG)
	SET_TARGET_PROPERTIES(vermont PROPERTIES LINK_FLAGS "-g -pg")
ELSE (DEBUG)
	REMOVE_DEFINITIONS(-O0 -g -pg -Wall -Werror -DDEBUG)
	ADD_DEFINITIONS(-O3)
	SET_TARGET_PROPERTIES(vermont PROPERTIES LINK_FLAGS "")
ENDIF (DEBUG)


### IP_HEADER_OFFSET

SET(IP_HEADER_OFFSET 14 CACHE STRING "Start position of the IP header in an ethernet frame in Bytes. This value needs to be adjusted according to the network monitored. The default value is 14 for ethernet devices. Other common values are 4 (BSD loop back device) and 18 (Ethernet VLAN)")
ADD_DEFINITIONS(-DIP_HEADER_OFFSET=${IP_HEADER_OFFSET})


### PCAP_MAX_CAPTURE_LENGTH

SET(PCAP_MAX_CAPTURE_LENGTH 128 CACHE STRING "Maximum PCAP packet capture length (this amount of bytes is always allocated for each packet, the smaller the better!)")
ADD_DEFINITIONS(-DPCAP_MAX_CAPTURE_LENGTH=${PCAP_MAX_CAPTURE_LENGTH})

### PCAP_DEFAULT_CAPTURE_LENGTH

SET(PCAP_DEFAULT_CAPTURE_LENGTH 128 CACHE STRING "Default PCAP packet capture length (default value for PCAP snaplen if not defined in configuration)")
ADD_DEFINITIONS(-DPCAP_DEFAULT_CAPTURE_LENGTH=${PCAP_DEFAULT_CAPTURE_LENGTH})


### SUPPORT_NETFLOWV9

OPTION(SUPPORT_NETFLOWV9 "Enable NetFlow version 9 support" ON)
IF (SUPPORT_NETFLOWV9)
	ADD_DEFINITIONS(-DSUPPORT_NETFLOWV9)
ELSE (SUPPORT_NETFLOWV9)
	REMOVE_DEFINITIONS(-DSUPPORT_NETFLOWV9)
ENDIF (SUPPORT_NETFLOWV9)


### MySQL

OPTION(SUPPORT_MYSQL "Enable dbwriter/dbreader support" OFF)
IF (SUPPORT_MYSQL)
	FIND_PACKAGE(MySQL REQUIRED)
	MARK_AS_ADVANCED(
		MYSQL_ADD_INCLUDE_DIR
		MYSQL_ADD_LIBRARY
		MYSQL_CONFIG
		MYSQL_CONFIG_PREFER_PATH
	)
	IF (NOT MYSQL_FOUND)
		MESSAGE(FATAL_ERROR "Could not find MySQL libraries.")
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

### PostgreSQL

OPTION(SUPPORT_PGSQL "Enable PostgreSQL support" OFF)
IF (SUPPORT_PGSQL)
	FIND_PACKAGE(PostgreSQL REQUIRED)
	IF (NOT POSTGRESQL_FOUND)
		MESSAGE(FATAL_ERROR "Could not find PostgreSQL libraries.")
	ENDIF (NOT POSTGRESQL_FOUND)
ENDIF (SUPPORT_PGSQL)
IF (POSTGRESQL_FOUND)
	MESSAGE(STATUS "Found PostgreSQL libraries")
	ADD_DEFINITIONS(-DPG_SUPPORT_ENABLED)
	INCLUDE_DIRECTORIES(${POSTGRESQL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${POSTGRESQL_LIBRARIES}
	)
ELSE (POSTGRESQL_FOUND)
	REMOVE_DEFINITIONS(-DPG_SUPPORT_ENABLED)
ENDIF (POSTGRESQL_FOUND)


### libpcap-mmap

OPTION(USE_PCAPMMAP "Use libpcap-mmap." OFF)
MARK_AS_ADVANCED(
	PCAP_LIBRARY_REGULAR
	PCAP_LIBRARY_MMAP
	PCAP_INCLUDE_PATH
)

IF (USE_PCAPMMAP)
    FIND_LIBRARY(PCAP_LIBRARY_MMAP NAMES pcap pcap-mmap PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/libpcap-mmap ${VERMONT_SOURCE_DIR}/../libpcap-mmap ${VERMONT_SOURCE_DIR}/../../../trunk/libpcap-mmap NO_DEFAULT_PATH)
	FIND_PATH(PCAP_INCLUDE_PATH NAMES pcap.h PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/libpcap-mmap ${VERMONT_SOURCE_DIR}/../libpcap-mmap ${VERMONT_SOURCE_DIR}/../../../trunk/libpcap-mmap NO_DEFAULT_PATH)
    SET(PCAP_LIBRARY "${PCAP_LIBRARY_MMAP}")
    IF (PCAP_LIBRARY_MMAP)
        SET(PCAP_LIBRARY "${PCAP_LIBRARY_MMAP}")
		INCLUDE_DIRECTORIES(${PCAP_INCLUDE_PATH})
    ELSE (PCAP_LIBRARY_MMAP)
        MESSAGE(FATAL_ERROR "Could not find libpcap-mmap. This script also looks in directory ./libpcap-mmap, so maybe you want to create a symlink to the directory of the library.")
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

### sctp
OPTION(SUPPORT_SCTP "Support SCTP transport protocol" ON)
IF (SUPPORT_SCTP)
    FIND_PACKAGE(Sctp REQUIRED)
    MARK_AS_ADVANCED(
        SCTP_LIBRARIES
        SCTP_INCLUDE_DIR
    )
    ADD_DEFINITIONS(-DSUPPORT_SCTP)
ELSE (SUPPORT_SCTP)
    REMOVE_DEFINITIONS(-DSUPPORT_SCTP)
ENDIF (SUPPORT_SCTP)

### connection filter

OPTION(CONNECTION_FILTER "Enables/disables the connection filter." OFF)
OPTION(USE_GSL "Enables/disables GSL in connectionflter." ON)

IF (CONNECTION_FILTER AND NOT USE_GSL)
       MESSAGE(FATAL_ERROR "GSL is needed for Connectionfilter at the moment.
       You cannot have -DCONNECTION_FILTER=ON and -DUSE_GSL=OFF")
ENDIF (CONNECTION_FILTER AND NOT USE_GSL)

IF (CONNECTION_FILTER)
	### gsl
	IF (USE_GSL)
		FIND_PACKAGE(GSL)
		MARK_AS_ADVANCED(
			GSL_INCLUDE_DIR
			GSL_LIBRARIES
			GSL_LIBRARY
			BLAS_LIBRARY
		)	
		IF (GSL_FOUND)
			MESSAGE(STATUS "GNU scientific library found")
		ELSE (GSL_FOUND)
			MESSAGE(FATAL_ERROR "GNU scientific library not found. Please
			install the library or use -DCONNECTION_FILTER=OFF and -DUSE_GSL=OFF")
		ENDIF (GSL_FOUND)
		ADD_DEFINITIONS(-DHAVE_GSL)
		TARGET_LINK_LIBRARIES(vermont ${GSL_LIBRARIES})
	ENDIF (USE_GSL)
	ADD_DEFINITIONS(-DHAVE_CONNECTION_FILTER)
ENDIF(CONNECTION_FILTER)

### tools

OPTION(WITH_TOOLS "Build misc tools." ON)
IF (WITH_TOOLS)
	SUBDIRS(tools)
	SUBDIR_DEPENDS(tools src)
ELSE (WITH_TOOLS)
ENDIF (WITH_TOOLS)


### tests

OPTION(WITH_TESTS "Build unit tests." OFF)
IF (WITH_TESTS)
	SUBDIRS(src/tests)
ELSE (WITH_TESTS)
ENDIF (WITH_TESTS)


### IPFIXLOLIB_RAWDIR_SUPPORT

OPTION(IPFIXLOLIB_RAWDIR_SUPPORT "Enable ipfix rawdir support" OFF)
IF (IPFIXLOLIB_RAWDIR_SUPPORT)
	ADD_DEFINITIONS(-DIPFIXLOLIB_RAWDIR_SUPPORT)
ELSE (IPFIXLOLIB_RAWDIR_SUPPORT)
	REMOVE_DEFINITIONS(-DIPFIXLOLIB_RAWDIR_SUPPORT)
ENDIF (IPFIXLOLIB_RAWDIR_SUPPORT)

