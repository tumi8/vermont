bin_PROGRAMS=test_concentrator

test_concentrator_SOURCES=test_concentrator.cpp ../msg.h ../msg.cc

AM_CFLAGS=-I$(top_srcdir) -I$(top_srcdir)/concentrator
AM_CXXFLAGS=$(AM_CFLAGS)
AM_LDFLAGS=-L$(top_builddir)/concentrator -L$(top_builddir)/ipfixlolib -lpthread -lpcap \
	$(BOOST_LDFLAGS) \
	$(BOOST_FILESYSTEM_LIB)

test_concentrator_LDADD=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/ipfixlolib/libipfixlo.a $(top_builddir)/sampler/libsampler.a
test_concentrator_DEPENDENCIES=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/ipfixlolib/libipfixlo.a $(top_builddir)/sampler/libsampler.a

