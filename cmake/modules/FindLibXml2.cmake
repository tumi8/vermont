# This line added for distribution with Vermont:
MESSAGE(STATUS "Using bundled FindLibXml2.cmake...")

FIND_PATH(
  LIBXML2_INCLUDE_DIR
  libxml/parser.h libxml/tree.h tree.h parser.h
  /usr/include/ /usr/include/libxml2/
  /usr/local/include/ /usr/local/include/libxml2/
  )

FIND_LIBRARY(
  LIBXML2_LIBRARIES NAMES xml2 xml libxml2
  PATHS /usr/lib/ /usr/local/lib/
  PATH_SUFFIXES libxml2
  )

IF (LIBXML2_INCLUDE_DIR)
  MESSAGE(STATUS "Found libxml2 include dirs")
  # If we don't add /usr/local/include to the include paths,
  # all code which uses libxml2 on FreeBSD won't compile
  INCLUDE_DIRECTORIES("/usr/local/include")
ELSE (LIBXML2_INCLUDE_DIR)
  MESSAGE(STATUS "Could not find libxml2 include dirs")
ENDIF(LIBXML2_INCLUDE_DIR)

IF (LIBXML2_LIBRARIES)
  MESSAGE(STATUS "Found libxml2 libraries")
ELSE (LIBXML2_LIBRARIES)
  MESSAGE(STATUS "Could not find libxml2 libraries")
ENDIF(LIBXML2_LIBRARIES)

IF (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
  MESSAGE(STATUS "Found libxml2")
ELSE (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
  MESSAGE(FATAL_ERROR "ERROR: Could not find libxml2. Please install the library.")
ENDIF (LIBXML2_INCLUDE_DIR AND LIBXML2_LIBRARIES)
