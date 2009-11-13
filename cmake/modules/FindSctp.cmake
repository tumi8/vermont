# This line added for distribution with Vermont:
MESSAGE(STATUS "Using bundled FindSctp.cmake...")


FIND_PATH(
  SCTP_INCLUDE_DIR
  sctp.h
  /usr/include/ /usr/include/sctp/ /usr/include/netinet
  /usr/local/include/ /usr/local/include/sctp/ /usr/local/include/netinet/
  )


# check wether we have we have all necessary functions within our libc
INCLUDE(CheckFunctionExists)
CHECK_FUNCTION_EXISTS(sctp_sendmsg SCTP_SENDMSGV_FOUND) 

# check for external libsctp
FIND_LIBRARY(
  SCTP_LIBRARIES NAMES sctp
  PATHS /usr/lib/ /usr/local/lib/
  )

IF (SCTP_LIBRARIES OR  SCTP_SENDMSGV_FOUND)
  SET (FOUND_SCTP_LIBRARIES TRUE)
ENDIF (SCTP_LIBRARIES OR  SCTP_SENDMSGV_FOUND)

IF (SCTP_INCLUDE_DIR)
  MESSAGE(STATUS "Found sctp include dirs")
ELSE (SCTP_INCLUDE_DIR)
  MESSAGE(STATUS "Could not find sctp include dirs")
ENDIF(SCTP_INCLUDE_DIR)

IF (FOUND_SCTP_LIBRARIES)
  MESSAGE(STATUS "Found sctp libraries")
  SET(SCTP_FOUND TRUE)
ELSE (FOUND_SCTP_LIBRARIES)
  MESSAGE(STATUS "Could not find sctp libraries")
ENDIF(FOUND_SCTP_LIBRARIES)

IF (SCTP_INCLUDE_DIR AND FOUND_SCTP_LIBRARIES)
  MESSAGE(STATUS "Found sctp")
ELSE (SCTP_INCLUDE_DIR AND FOUND_SCTP_LIBRARIES)
  MESSAGE(FATAL_ERROR "ERROR: Could not find libsctp. Please install the library.")
ENDIF (SCTP_INCLUDE_DIR AND FOUND_SCTP_LIBRARIES)
