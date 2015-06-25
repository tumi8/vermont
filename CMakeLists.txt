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
CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

# TODO: Once all warnings are fixed add -Werror to both of these
# TODO: Bonus: Remove -Wno-unused-parameter and fix all code as this one gernerates hundreds of warnings
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu11 -Wall -pedantic -Wextra -Wno-unused-parameter"
    CACHE STRING
    "Recommended C Flags"
    FORCE)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11 -Wall -pedantic -Wextra -Wno-unused-parameter"
    CACHE STRING
    "Recommended C++ Flags"
    FORCE)

# Add our own specifc debug flag to the defaults
set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -O0 -DDEBUG" CACHE STRING "Recommended C Debug Flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -DDEBUG" CACHE STRING "Recommended C++ Debug Flags" FORCE)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release CACHE STRING
      "Choose the type of build, options are: None Debug Release RelWithDebInfo MinSizeRel."
      FORCE)
ENDIF(NOT CMAKE_BUILD_TYPE)
message("* Current build type is : ${CMAKE_BUILD_TYPE}")

# move some config clutter to the advanced section
MARK_AS_ADVANCED(
	CMAKE_BACKWARDS_COMPATIBILITY
	CMAKE_INSTALL_PREFIX
	EXECUTABLE_OUTPUT_PATH
	LIBRARY_OUTPUT_PATH
)

include(CppcheckTargets)

### basic modules

SUBDIRS(
	src
)

if(COMMAND cmake_policy)
	cmake_policy(SET CMP0003 OLD)
	cmake_policy(SET CMP0002 OLD)
	if (POLICY CMP0017)
	    cmake_policy(SET CMP0017 OLD)
	endif(POLICY CMP0017)
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

INSTALL(FILES README LICENSE
	DESTINATION share/doc/vermont
)

INSTALL(DIRECTORY configs/
	DESTINATION share/vermont/configs
)

add_cppcheck(vermont STYLE POSSIBLE_ERROR)

### doxygen

#FIND_PACKAGE(Doxygen REQUIRED)


### threads

FIND_PACKAGE(Threads REQUIRED)
TARGET_LINK_LIBRARIES(vermont
	${CMAKE_THREAD_LIBS_INIT}
)

### MongoDB

OPTION(SUPPORT_MONGO "Enable MongoDB support" OFF)
IF (SUPPORT_MONGO)
	FIND_PACKAGE(MONGO REQUIRED)
	IF (NOT MONGO_FOUND)
		MESSAGE(FATAL_ERROR "Could not find MongoDB libraries.")
	ENDIF (NOT MONGO_FOUND)
ENDIF (SUPPORT_MONGO)
IF (MONGO_FOUND)
	ADD_DEFINITIONS(-DMONGO_SUPPORT_ENABLED)
	### Mongo is currently forcing BOOST_FILE_SYSTEM v2 when its 
	### headers are included. If we use version v3 in the rest of
	### vermont, we will get compile time errors because of
	### redefinitions of certain boost internals. If we support
	### mongo, we therefore force v2 on the complete vermont
	### compile process. Remove this if v2 is dropped by the boost
	### people and mongo switched to v3
	ADD_DEFINITIONS(-DBOOST_FILESYSTEM_VERSION=2)

	IF (MONGO_VERSION_2)
	   ADD_DEFINITIONS(-DMONGO_VERSION_2)
	   MESSAGE(STATUS "Found MongoDB libraries (version 2)")
	ELSE (MONGO_VERSION_2)
	   MESSAGE(STATUS "Found MongoDB libraries")
	ENDIF (MONGO_VERSION_2)

	INCLUDE_DIRECTORIES(${MONGO_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${MONGO_LIBRARIES}
	)
ELSE (MONGO_FOUND)
	REMOVE_DEFINITIONS(-DMONGO_SUPPORT_ENABLED)
	REMOVE_DEFINITIONS(-DMONGO_VERSION_2)
	REMOVE_DEFINITIONS(-DBOOST_FILESYSTEM_VERSION)
ENDIF (MONGO_FOUND)

### Redis

OPTION(SUPPORT_REDIS "Enable Redis support (for flow-inspector integration)" OFF)
IF (SUPPORT_REDIS)
	FIND_PACKAGE(Redis REQUIRED)
	IF (NOT HIREDIS_FOUND)
		MESSAGE(FATAL_ERROR "Could not find hiredis client libraries.")
	ENDIF (NOT HIREDIS_FOUND)
ENDIF (SUPPORT_REDIS)
IF (HIREDIS_FOUND)
	ADD_DEFINITIONS(-DREDIS_SUPPORT_ENABLED)
	INCLUDE_DIRECTORIES(${HIREDIS_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${HIREDIS_LIBRARIES}
	)
ELSE (HIREDIS_FOUND)
	REMOVE_DEFINITIONS(-DREDIS_SUPPORT_ENABLED)
ENDIF (HIREDIS_FOUND)

### boost

FIND_PACKAGE(Boost REQUIRED)
MARK_AS_ADVANCED(
	Boost_INCLUDE_DIR
	Boost_REGEX_LIBRARY
  Boost_THREAD_LIBRARY
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
  FIND_LIBRARY(Boost_THREAD_LIBRARY NAMES boost_thread-mt boost_thread PATHS ${Boost_LIBRARY_DIRS})
  IF (NOT Boost_THREAD_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost thread library")
  ENDIF(NOT Boost_THREAD_LIBRARY)
  FIND_LIBRARY(Boost_FILESYSTEM_LIBRARY NAMES boost_filesystem-mt boost_filesystem PATHS ${Boost_LIBRARY_DIRS})
	IF (NOT Boost_FILESYSTEM_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost filesystem library")
	ENDIF(NOT Boost_FILESYSTEM_LIBRARY)
	FIND_LIBRARY(Boost_UNIT_TEST_FRAMEWORK_LIBRARY NAMES boost_unit_test_framework-mt boost_unit_test_framework PATHS ${Boost_LIBRARY_DIRS})
	IF (NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost unit test framework")
	ENDIF (NOT Boost_UNIT_TEST_FRAMEWORK_LIBRARY)
	FIND_LIBRARY(Boost_SYSTEM_LIBRARY NAMES boost_system-mt boost_system PATHS ${Boost_LIBRARY_DIRS})
	IF (NOT Boost_SYSTEM_LIBRARY)
		MESSAGE(FATAL_ERROR "Could not find boost system library")
	ENDIF (NOT Boost_SYSTEM_LIBRARY)
	TARGET_LINK_LIBRARIES(vermont
		${Boost_REGEX_LIBRARY}
		${Boost_FILESYSTEM_LIBRARY}
		${Boost_SYSTEM_LIBRARY}
	)
  IF (SUPPORT_MONGO) 
	  TARGET_LINK_LIBRARIES(vermont
		  ${Boost_THREAD_LIBRARY}
    )
  ENDIF (SUPPORT_MONGO)

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

### export time sanity checks
OPTION(EXPORT_TIME_SANITY_CHECK "Enable sanity checks on arrival of flows based on export time" OFF)
IF(EXPORT_TIME_SANITY_CHECK)
	ADD_DEFINITIONS(-DEXPORT_TIME_SANITY_CHECK)
ENDIF(EXPORT_TIME_SANITY_CHECK)

### PCAP_MAX_CAPTURE_LENGTH

SET(PCAP_MAX_CAPTURE_LENGTH 128 CACHE STRING "Maximum PCAP packet capture length (this amount of bytes is always allocated for each packet, the smaller the better!)")
ADD_DEFINITIONS(-DPCAP_MAX_CAPTURE_LENGTH=${PCAP_MAX_CAPTURE_LENGTH})

# TODO: there is a bug in the code that occurs then the MAX_CAPTURE_LENGTH is > 65000
#	that occurs in the offset calucation in PacketHashTable (the offset is a uint16_t that
#	overflows when MAX_CAPTURE_LENGTH is too big.
#	until we fixted the bug, we restrict ourselfs on on teh maximum capture length
IF (PCAP_MAX_CAPTURE_LENGTH GREATER 65000) 
	MESSAGE(FATAL_ERROR "PCAP_MAX_CAPTURE_LENGTH is too big. Make it <= 65000 or fix the bug described in CMakeLists.txt")
ENDIF(PCAP_MAX_CAPTURE_LENGTH GREATER 65000)

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
IF (MYSQL_FOUND AND SUPPORT_MYSQL)
	MESSAGE(STATUS "Found MySQL libraries")
	ADD_DEFINITIONS(-DDB_SUPPORT_ENABLED)
	INCLUDE_DIRECTORIES(${MYSQL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${MYSQL_LIBRARIES}
	)
ELSE (MYSQL_FOUND AND SUPPORT_MYSQL)
	REMOVE_DEFINITIONS(-DDB_SUPPORT_ENABLED)
ENDIF (MYSQL_FOUND AND SUPPORT_MYSQL)

### PostgreSQL
# Bug in Ubuntu 14.04 does not find PSQL Libs
# https://bugs.launchpad.net/ubuntu/+source/cmake/+bug/1317135
set(PostgreSQL_ADDITIONAL_SEARCH_PATHS "/usr/include/postgresql/9.3/server")

OPTION(SUPPORT_POSTGRESQL "Enable PostgreSQL support" OFF)
IF (SUPPORT_POSTGRESQL)
	FIND_PACKAGE(PostgreSQL REQUIRED)
	IF (NOT PostgreSQL_FOUND)
		MESSAGE(FATAL_ERROR "Could not find PostgreSQL libraries.")
	ENDIF (NOT PostgreSQL_FOUND)
ENDIF (SUPPORT_POSTGRESQL)
IF (PostgreSQL_FOUND)
	MESSAGE(STATUS "Found PostgreSQL libraries")
	ADD_DEFINITIONS(-DPG_SUPPORT_ENABLED)
	INCLUDE_DIRECTORIES(${PostgreSQL_INCLUDE_DIRS})
	TARGET_LINK_LIBRARIES(vermont
		${PostgreSQL_LIBRARIES}
	)
ELSE (PostgreSQL_FOUND)
	REMOVE_DEFINITIONS(-DPG_SUPPORT_ENABLED)
ENDIF (PostgreSQL_FOUND)

### Oracle

OPTION(SUPPORT_ORACLE "Enable Oracle support" OFF)
IF (SUPPORT_ORACLE)
	FIND_PACKAGE(Oracle REQUIRED)
	IF (NOT ORACLE_FOUND)
		MESSAGE(FATAL_ERROR "Could not find Oracle libraries.")
	ENDIF (NOT ORACLE_FOUND)
ENDIF (SUPPORT_ORACLE)
IF (ORACLE_FOUND)
	MESSAGE(STATUS "Found Oracle libraries")
	ADD_DEFINITIONS(-DORACLE_SUPPORT_ENABLED)
	INCLUDE_DIRECTORIES(${ORACLE_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont
		${ORACLE_CONNECTION_LIBRARIES}
	)
	TARGET_LINK_LIBRARIES(vermont
		${ORACLE_CLIENT_LIBRARIES}
	)	
ELSE (ORACLE_FOUND)
	REMOVE_DEFINITIONS(-DORACLE_SUPPORT_ENABLED)
ENDIF (ORACLE_FOUND)

### libpcap-mmap

OPTION(USE_PCAPMMAP "Use libpcap-mmap." OFF)
MARK_AS_ADVANCED(
	PCAP_LIBRARY_REGULAR
	PCAP_LIBRARY_MMAP
	PCAP_INCLUDE_PATH
)

OPTION(USE_PFRING "Use pf_ring optimized libpcap." OFF)
MARK_AS_ADVANCED(
	PCAP_LIBRARY_PFRING
	PFRING_LIBRARY
)

IF (USE_PCAPMMAP)

    IF (USE_PFRING)
        MESSAGE(FATAL_ERROR "Both USE_PCAPMMAP and USE_PFRING have been selected. At most one of these options may be selected!")
    ENDIF (USE_PFRING)

    FIND_LIBRARY(PCAP_LIBRARY_MMAP NAMES pcap pcap-mmap PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/libpcap-mmap ${VERMONT_SOURCE_DIR}/../libpcap-mmap ${VERMONT_SOURCE_DIR}/../../../trunk/libpcap-mmap NO_DEFAULT_PATH)
	FIND_PATH(PCAP_INCLUDE_PATH NAMES pcap.h PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/libpcap-mmap ${VERMONT_SOURCE_DIR}/../libpcap-mmap ${VERMONT_SOURCE_DIR}/../../../trunk/libpcap-mmap NO_DEFAULT_PATH)
    SET(PCAP_LIBRARY "${PCAP_LIBRARY_MMAP}")
    IF (PCAP_LIBRARY_MMAP)
        SET(PCAP_LIBRARY "${PCAP_LIBRARY_MMAP}")
		INCLUDE_DIRECTORIES(${PCAP_INCLUDE_PATH})
    ELSE (PCAP_LIBRARY_MMAP)
        MESSAGE(FATAL_ERROR "Could not find libpcap-mmap. This script also looks in directory ./libpcap-mmap, so maybe you want to create a symlink to the directory of the library.")
    ENDIF (PCAP_LIBRARY_MMAP)
    TARGET_LINK_LIBRARIES(vermont ${PCAP_LIBRARY})

ELSEIF (USE_PFRING)

    FIND_LIBRARY(PCAP_LIBRARY_PFRING NAMES pcap PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/PF_RING/userland/libpcap-1.0.0-ring ${VERMONT_SOURCE_DIR}/../PF_RING/userland/libpcap-1.0.0-ring NO_DEFAULT_PATH)
    IF (NOT PCAP_LIBRARY_PFRING)
        MESSAGE(FATAL_ERROR "Could not find libpcap-ring. This script looks in directories ${VERMONT_SOURCE_DIR}/PF_RING/userland/libpcap-1.0.0-ring and ${VERMONT_SOURCE_DIR}/../PF_RING/userland/libpcap-1.0.0-ring, so maybe you want to create a symlink to the directory of the library.")
    ENDIF (NOT PCAP_LIBRARY_PFRING)
    FIND_LIBRARY(PFRING_LIBRARY NAMES pfring PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/PF_RING/userland/lib ${VERMONT_SOURCE_DIR}/../PF_RING/userland/lib NO_DEFAULT_PATH)
    IF (NOT PFRING_LIBRARY)
        MESSAGE(FATAL_ERROR "Could not find libpfring. This script looks in directories ${VERMONT_SOURCE_DIR}/PF_RING/userland/lib and ${VERMONT_SOURCE_DIR}/../PF_RING/userland/lib, so maybe you want to create a symlink to the directory of the library.")
    ENDIF (NOT PFRING_LIBRARY)
    FIND_PATH(PCAP_INCLUDE_PATH NAMES pcap.h PATHS ${VERMONT_SOURCE_DIR} ${VERMONT_SOURCE_DIR}/PF_RING/userland/libpcap-1.0.0-ring ${VERMONT_SOURCE_DIR}/../PF_RING/userland/libpcap-1.0.0-ring NO_DEFAULT_PATH)
    TARGET_LINK_LIBRARIES(vermont ${PCAP_LIBRARY_PFRING})
    TARGET_LINK_LIBRARIES(vermont ${PFRING_LIBRARY})
    INCLUDE_DIRECTORIES(${PCAP_INCLUDE_PATH})

ELSE (USE_PCAPMMAP)

    FIND_LIBRARY(PCAP_LIBRARY_REGULAR NAMES pcap)
    IF (PCAP_LIBRARY_REGULAR)
        SET(PCAP_LIBRARY "${PCAP_LIBRARY_REGULAR}")
    ELSE (PCAP_LIBRARY_REGULAR)
        MESSAGE(FATAL_ERROR "Could not find libpcap")
    ENDIF (PCAP_LIBRARY_REGULAR)
    TARGET_LINK_LIBRARIES(vermont ${PCAP_LIBRARY})

ENDIF (USE_PCAPMMAP)



IF (USE_PFRING)
ELSE (USE_PFRING)
    FIND_LIBRARY(PCAP_LIBRARY_REGULAR NAMES pcap)
    IF (PCAP_LIBRARY_REGULAR)
    	TARGET_LINK_LIBRARIES(vermont ${PCAP_LIBRARY_REGULAR})
    ELSE (PCAP_LIBRARY_REGULAR)
        MESSAGE(FATAL_ERROR "Could not find libpcap.")
    ENDIF (PCAP_LIBRARY_REGULAR)
ENDIF (USE_PFRING)

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

### openssl

OPTION(SUPPORT_DTLS "Enables/Disables encryption support for IPFIX messages." OFF)
IF (SUPPORT_DTLS)
	FIND_PACKAGE(OpenSSL)
	IF (NOT OPENSSL_FOUND)
		MESSAGE(FATAL_ERROR "Could not find openssl. Please install the library or turn off SUPPORT_DTLS")
	ENDIF (NOT OPENSSL_FOUND)
	INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
	TARGET_LINK_LIBRARIES(vermont ${OPENSSL_LIBRARIES})
	ADD_DEFINITIONS(-DSUPPORT_DTLS)

	## check wether there is a libcrypto
	# FIND_PACKAGE(Crypto)
	# IF (CRYPTO_FOUND)
	# 	TARGET_LINK_LIBRARIES(vermont ${CRYPTO_LIBRARIES})
	# ENDIF (CRYPTO_FOUND)
ELSE(SUPPORT_DTLS)
	REMOVE_DEFINITIONS(-DSUPPORT_DLTS)
ENDIF(SUPPORT_DTLS)

OPTION(SUPPORT_DTLS_OVER_SCTP "Enables/Disables DTLS on SCTP streams." OFF)
IF (SUPPORT_DTLS_OVER_SCTP)
	IF (SUPPORT_SCTP AND SUPPORT_DTLS)
		# Make sure OpenSSL was compiled with SCTP support enabled
		INCLUDE(CheckSymbolExists)
		SET(CMAKE_REQUIRED_INCLUDES ${OPENSSL_INCLUDE_DIR})
		# OpenSSL #defines OPENSSL_SCTP if it supports SCTP
		CHECK_SYMBOL_EXISTS(OPENSSL_SCTP "openssl/ssl.h" OPENSSL_SUPPORTS_SCTP)
		SET(CMAKE_REQUIRED_INCLUDES)
		IF (NOT OPENSSL_SUPPORTS_SCTP)
			MESSAGE(FATAL_ERROR "OpenSSL built without SCTP support. Rebuild OpenSSL with SCTP support or turn off SUPPORT_DTLS_OVER_SCTP")
		ENDIF (NOT OPENSSL_SUPPORTS_SCTP)
		ADD_DEFINITIONS(-DSUPPORT_DTLS_OVER_SCTP)
	ELSE (SUPPORT_SCTP AND SUPPORT_DTLS)
		MESSAGE(FATAL_ERROR "SUPPORT_DTLS_OVER_SCTP can only be enabled together with SUPPORT_SCTP and SUPPORT_DTLS")
		REMOVE_DEFINITIONS(-DSUPPORT_DTLS_OVER_SCTP)
	ENDIF(SUPPORT_SCTP AND SUPPORT_DTLS)
ELSE (SUPPORT_DTLS_OVER_SCTP)
	REMOVE_DEFINITIONS(-DSUPPORT_DTLS_OVER_SCTP)
ENDIF (SUPPORT_DTLS_OVER_SCTP)

### tools

OPTION(WITH_TOOLS "Build misc tools." ON)
IF (WITH_TOOLS)
	ADD_SUBDIRECTORY(tools)
ELSE (WITH_TOOLS)
ENDIF (WITH_TOOLS)

OPTION(DISABLE_MEMMANAGER "Disable custom memory manager (good for finding leaks with valgrind." OFF)
IF (DISABLE_MEMMANAGER)
	if (NOT DEBUG)
		MESSAGE(FATAL_ERROR "Flag DISABLE_MEMMANAGER may only be set if flag DEBUG is set")
	ENDIF (NOT DEBUG)
	ADD_DEFINITIONS(-DIM_DISABLE)
ENDIF (DISABLE_MEMMANAGER)


### IPFIXLOLIB_RAWDIR_SUPPORT

OPTION(IPFIXLOLIB_RAWDIR_SUPPORT "Enable ipfix rawdir support" OFF)
IF (IPFIXLOLIB_RAWDIR_SUPPORT)
	ADD_DEFINITIONS(-DIPFIXLOLIB_RAWDIR_SUPPORT)
ELSE (IPFIXLOLIB_RAWDIR_SUPPORT)
	REMOVE_DEFINITIONS(-DIPFIXLOLIB_RAWDIR_SUPPORT)
ENDIF (IPFIXLOLIB_RAWDIR_SUPPORT)

