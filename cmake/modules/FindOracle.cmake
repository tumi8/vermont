# - Find Oracle
# Find the Oracle includes and client library
# This module defines
#  ORACLE_INCLUDE_DIR, where to find occi.h
#  ORACLE_LIBRARIES, the libraries needed to use ORACLE.
#  ORACLE_FOUND, If false, do not try to use Oracle.
#
# Copyright (c) 2011, Philipp Fehre, <philipp.fehre@googlemail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

# Add the oracle include paths here

if(ORACLE_INCLUDE_DIR AND ORACLE_CLIENT_LIBRARIES AND ORACLE_CONNECTION_LIBRARIES)
   set(ORACLE_FOUND TRUE)

else(ORACLE_INCLUDE_DIR AND ORACLE_CLIENT_LIBRARIES AND ORACLE_CONNECTION_LIBRARIES)

	find_path(ORACLE_INCLUDE_DIR occi.h
	  /usr/lib/oracle/xe/app/oracle/product/*/client/rdbms/public
	  /usr/include/oracle/*/client
	  /opt/oracle/product/*/client/rdbms/public
	  /opt/oracle/*/client/
    )

  find_library(ORACLE_CLIENT_LIBRARIES NAMES clntsh libclntsh
    PATHS
    /usr/lib/oracle/xe/app/oracle/product/*/client/lib
    /usr/lib/oracle/*/client/lib
    /opt/oracle/product/*/client/lib
    /opt/oracle/*/client/lib
    )

  find_library(ORACLE_CONNECTION_LIBRARIES NAMES occi libocci
    PATHS
    /usr/lib/oracle/xe/app/oracle/product/*/client/lib
    /usr/lib/oracle/*/client/lib
    /opt/oracle/product/*/client/lib
    /opt/oracle/*/client/lib
    )
      
  if(ORACLE_INCLUDE_DIR AND ORACLE_CLIENT_LIBRARIES AND ORACLE_CONNECTION_LIBRARIES)
    set(ORACLE_FOUND TRUE)
    message(STATUS "Found Oracle: ${ORACLE_INCLUDE_DIR}, ${ORACLE_CLIENT_LIBRARIES}, ${ORACLE_CONNECTION_LIBRARIES}")
    INCLUDE_DIRECTORIES(${ORACLE_INCLUDE_DIR})
  else(ORACLE_INCLUDE_DIR AND ORACLE_CLIENT_LIBRARIES AND ORACLE_CONNECTION_LIBRARIES)
    set(ORACLE_FOUND FALSE)
    message(STATUS "Oracle not found.")
  endif(ORACLE_INCLUDE_DIR AND ORACLE_CLIENT_LIBRARIES AND ORACLE_CONNECTION_LIBRARIES)

  mark_as_advanced(ORACLE_INCLUDE_DIR ORACLE_CLIENT_LIBRARIES ORACLE_CONNECTION_LIBRARIES )

endif(ORACLE_INCLUDE_DIR AND ORACLE_CLIENT_LIBRARIES AND ORACLE_CONNECTION_LIBRARIES)
