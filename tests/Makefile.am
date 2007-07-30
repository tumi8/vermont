bin_PROGRAMS=vermonttest

vermonttest_SOURCES=test_concentrator.cpp common/msg.h AggregationPerfTest.cpp AggregationPerfTest.h common/Time.h VermontTest.cpp VermontTest.h

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
	-lpthread \
	$(LPCAP)

vermonttest_DEPENDENCIES= \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a \
	$(top_builddir)/common/libcommon.a 

