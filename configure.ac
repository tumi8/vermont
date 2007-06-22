AC_INIT
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE(vermont, 0.9.1)

# turn on debug mode
debug="no"
AC_ARG_ENABLE([debug],
              [AS_HELP_STRING([--enable-debug],
                              [enable DPRINT debug messages, debug info and no optimizations. This will make VERMONT very noisy])],
              [debug="$enableval"], [])
if test "x$debug" = "xyes"; then
  AC_DEFINE(DEBUG)
  CFLAGS="$CFLAGS -O0 -g"
  CXXFLAGS="$CXXFLAGS -O0 -g"
else
  CFLAGS="$CFLAGS -O3"
  CXXFLAGS="$CXXFLAGS -O3"
fi


AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

AC_CHECK_HEADERS(stdio.h unistd.h signal.h stdlib.h stdint.h string.h)
AC_CHECK_HEADERS(time.h assert.h stdint.h errno.h netinet/in.h arpa/inet.h)
AC_CHECK_HEADERS(sys/socket.h sys/types.h sys/uio.h sys/time.h netdb.h semaphore.h)

# IPHeaderOffset switch
AC_ARG_WITH([ipheader-offset],
            [AS_HELP_STRING([--with-ipheader-offset=ARG],
              [specify start position of the IP header in an ethernet frame in Bytes])],
            [],
            [with_ipheader_offset=check])
AS_IF([test "x$with_ipheader_offset" = "xcheck"],
      [AC_MSG_NOTICE([Assuming IP header offset is 14 Bytes])
       AC_DEFINE(IP_HEADER_OFFSET, 14)],
      [AC_MSG_NOTICE([IP header offset set to $with_ipheader_offset Bytes])
       AC_DEFINE_UNQUOTED(IP_HEADER_OFFSET, $with_ipheader_offset)])

# enable NETFLOWv9 support
AC_ARG_ENABLE([netflow-v9],
              [AS_HELP_STRING([--enable-netflow-v9],
                              [enable NetFlow version 9 support (turned on by default)])])
AS_IF([test "x$enable_netflow_v9" = "xyes" -o "x$enable_netflow_v9" = "x"],
      [AC_MSG_NOTICE([NetFlow version 9 support enabled])
       AC_DEFINE(SUPPORT_NETFLOWV9)],
      [AC_MSG_NOTICE([NetFlow version 9 support disabled])])

# abort if anybody tries to use --enable-dprint
AC_ARG_ENABLE([dprint],
              [AS_HELP_STRING([--enable-dprint],
                              [no longer used])],
              [AC_MSG_ERROR([*** --enable-dprint was replaced by --enable-debug])])

# Check for LibXML
AM_PATH_XML2(2.4.0,,AC_MSG_ERROR([*** LibXML version $XML_VERSION not found!]))

# check for pthread
ACX_PTHREAD

# enable dbwriter/dbreader support
# check for mysql client liebrary if enabled
AC_ARG_ENABLE([db],
              [AS_HELP_STRING([--enable-db])],
              [enable_db_support=check])
AS_IF([test "x$enable_db_support" = "xcheck"],
      [AC_MSG_NOTICE([Enable dbwriter/dbreader support])
       AC_DEFINE(DB_SUPPORT_ENABLED)
       #any mysql-version should be adequate
       MYSQL_FOUND=AX_LIB_MYSQL([0.1])
       if test "$MYSQL_VERSION" = ""; then
		AC_MSG_ERROR([*** mysql client library not found. Please install it.])
       fi
      ],
      [AC_MSG_NOTICE([DbReader/DbWriter support disabled])])

# check for libpcap
AC_CHECK_HEADER([pcap.h],
                [],
                [AC_MSG_ERROR([*** could not find pcap.h. Please install libpcap.])])

# configure option for libpcap-mmap
pcapmmap="no"
AC_ARG_ENABLE([pcapmmap],
              [AS_HELP_STRING([--enable-pcapmmap],
                              [enable libpcap-mmap, libpcap.a MUST be available in directory ../libpcap-mmap])],
              [pcapmmap="$enableval"], [])
AM_CONDITIONAL(PCAPMMAP, test x$pcapmmap = xyes)
# only check for pcap if pcapmmap is not defined
# (else the custom library is used and will not be checked here)
if test "x$pcapmmap" = "xno"; then
    AC_CHECK_LIB([pcap], [pcap_open_live],
		[LDFLAGS="$LDFLAGS -lpcap"
		AC_SUBST(LDFLAGS)],
		[AC_MSG_ERROR([*** could not find libpcap. Please install it.])])
    AC_MSG_NOTICE([Using standard system libpcap])
else
    AC_MSG_NOTICE([Using libpcap-mmap of vermont])
fi

# check for Boost headers
AX_BOOST_BASE([1.33.1])

# check for Boost Regex
AX_BOOST_REGEX
if test "x$BOOST_REGEX_LIB" = "x"; then
	AC_MSG_ERROR([*** Boost Regex libraries not installed. Please install them.])
fi

AC_CONFIG_FILES([Makefile concentrator/Makefile sampler/Makefile ipfixlolib/Makefile tools/Makefile])

AC_OUTPUT

if test "x$debug" = "xyes"; then
	AC_MSG_RESULT([])
	AC_MSG_RESULT([*******************************************************************************])
	AC_MSG_RESULT([* Warning: You are building a non-optimized version of VERMONT for debugging! *])
	AC_MSG_RESULT([*******************************************************************************])
	AC_MSG_RESULT([])
fi

