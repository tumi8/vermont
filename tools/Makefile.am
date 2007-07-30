bin_PROGRAMS=testCollector

testCollector_SOURCES=testCollector.cpp ../common/msg.h

AM_CFLAGS=-I$(top_srcdir) -I$(top_srcdir)/concentrator
AM_CXXFLAGS=$(AM_CFLAGS)
AM_LDFLAGS=-L$(top_builddir)/concentrator -L$(top_builddir)/ipfixlolib -lpthread -lpcap
testCollector_LDADD=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/ipfixlolib/libipfixlo.a $(top_builddir)/common/libcommon.a
testCollector_DEPENDENCIES=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/ipfixlolib/libipfixlo.a $(top_builddir)/common/libcommon.a
