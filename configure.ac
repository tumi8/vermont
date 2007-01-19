AC_INIT
AC_PREREQ(2.59)
AM_INIT_AUTOMAKE(vermont, 0.9)
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

# turn on debug mode
AC_ARG_ENABLE([dprint],
              [AS_HELP_STRING([--enable-dprint],
                              [enable DPRINT debug messages. This will make VERMONT very noisy])],
              [AC_DEFINE(DEBUG)])

AM_PATH_XML2(2.4.0,,AC_MSG_ERROR([*** LibXML version $XML_VERSION not found!]))


# check for pthread
ACX_PTHREAD

# any mysql-version should be adequate
MYSQL_FOUND=AX_LIB_MYSQL([0.1])
if test "$MYSQL_VERSION" = ""; then
	AC_MSG_ERROR([*** mysql client library not found. Please install it.])
fi

# check for libpcap
AC_CHECK_HEADER([pcap.h],
                [],
                [AC_MSG_ERROR([*** could not find pcap.h. Please install libpcap.])])
AC_CHECK_LIB([pcap], [pcap_open_live],
             [LDFLAGS="$LDFLAGS -lpcap"
              AC_SUBST(LDFLAGS)],
             [AC_MSG_ERROR([*** could not find libpcap. Please install it.])])

AC_CONFIG_FILES([Makefile concentrator/Makefile sampler/Makefile ipfixlolib/Makefile tools/Makefile])

AC_OUTPUT
