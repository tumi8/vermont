AC_INIT
AC_PREREQ(2.57)
AM_INIT_AUTOMAKE(vermont, 0.8)
AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AM_PATH_XML2(2.4.0,,AC_MSG_ERROR([*** LibXML version $XML_VERSION not found!]))
ACX_PTHREAD


AC_MSG_WARN([We lack a check for libpcap. Please install the library, if you haven't done this already.])

AC_CONFIG_FILES([Makefile concentrator/Makefile sampler/Makefile ipfixlolib/Makefile tools/Makefile])

AC_OUTPUT
