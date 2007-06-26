bin_PROGRAMS=test_concentrator

test_concentrator_SOURCES=test_concentrator.cpp \
	../msg.h ../msg.cc

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
	$(BOOST_REGEX_LIB)

test_concentrator_LDADD= \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a \
	-lpthread \
	$(LPCAP)

test_concentrator_DEPENDENCIES= \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a
