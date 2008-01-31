# This line added for distribution with Vermont:
MESSAGE(STATUS "Using bundled FindSctp.cmake...")

FIND_PATH(
  SCTP_INCLUDE_DIR
  sctp.h
  /usr/include/ /usr/include/sctp/ /usr/include/netinet
  /usr/local/include/ /usr/local/include/sctp/ /usr/local/include/netinet/
  )

FIND_LIBRARY(
  SCTP_LIBRARIES NAMES sctp
  PATHS /usr/lib/ /usr/local/lib/
  )

IF (SCTP_INCLUDE_DIR)
  MESSAGE(STATUS "Found sctp include dirs")
ELSE (SCTP_INCLUDE_DIR)
  MESSAGE(STATUS "Could not find sctp include dirs")
ENDIF(SCTP_INCLUDE_DIR)

IF (SCTP_LIBRARIES)
  MESSAGE(STATUS "Found sctp libraries")
ELSE (SCTP_LIBRARIES)
  MESSAGE(STATUS "Could not find sctp libraries")
ENDIF(SCTP_LIBRARIES)

IF (SCTP_INCLUDE_DIR AND SCTP_LIBRARIES)
  MESSAGE(STATUS "Found sctp")
ELSE (SCTP_INCLUDE_DIR AND SCTP_LIBRARIES)
  MESSAGE(FATAL_ERROR "ERROR: Could not find libsctp. Please install the library.")
ENDIF (SCTP_INCLUDE_DIR AND SCTP_LIBRARIES)
