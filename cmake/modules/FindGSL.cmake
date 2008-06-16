#################################### Locate gsl
FIND_PATH(
  GSL_INCLUDE_DIR
  gsl/gsl_cdf.h
  /usr/include/ /usr/include/gsl/
  /use/local/include/ /usr/local/include/gsl/
  )

FIND_LIBRARY(
  GSL_LIBRARY
  NAMES gsl
  PATHS /usr/lib /usr/local/lib
  PATH_SUFFIXES gsl
  )

FIND_LIBRARY(
  BLAS_LIBRARY
  NAMES gslcblas blas cblas
  PATHS /usr/lib /usr/local/lib
  PATH_SUFFIXES gsl blas cblas
  )

IF (GSL_LIBRARY AND BLAS_LIBRARY)
  SET(GSL_LIBRARIES ${GSL_LIBRARY} ${BLAS_LIBRARY})
ENDIF (GSL_LIBRARY AND BLAS_LIBRARY)

IF (GSL_INCLUDE_DIR AND GSL_LIBRARIES)
  SET(GSL_FOUND TRUE)
ENDIF (GSL_INCLUDE_DIR AND GSL_LIBRARIES)

