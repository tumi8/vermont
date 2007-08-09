bin_PROGRAMS=vermonttest

vermonttest_SOURCES=\
	AggregationPerfTest.cpp AggregationPerfTest.h \
	common/msg.h \
	common/Time.h \
	reconf/PacketSource.h reconf/PacketSource.h \
	reconf/Emitable.h \
	reconf/Destination.h \
	reconf/Source.h \
	reconf/PacketDestination.h \
	reconf/PacketSource.h \
	reconf/PacketConnectionQueue.h \
	test_concentrator.cpp \
	VermontTest.cpp VermontTest.h

AM_CFLAGS=-I$(top_srcdir) `xml2-config --cflags` $(MYSQL_CFLAGS) -Wall -Werror
AM_CXXFLAGS=$(AM_CFLAGS)
AM_LDFLAGS= \
	-L$(top_builddir)/concentrator \
	-L$(top_builddir)/sampler \
	-L$(top_builddir)/ipfixlolib \
	`xml2-config --libs` \
	$(MYSQL_LDFLAGS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_FILESYSTEM_LIB) \
	$(BOOST_REGEX_LIB) \
	$(BOOST_UNIT_TEST_FRAMEWORK_LIB)


vermonttest_LDADD= \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a \
	$(top_builddir)/common/libcommon.a \
	$(top_builddir)/reconf/libreconf.a \
	-lpthread \
	$(LPCAP)

vermonttest_DEPENDENCIES= \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a \
	$(top_builddir)/reconf/libreconf.a \
	$(top_builddir)/common/libcommon.a

