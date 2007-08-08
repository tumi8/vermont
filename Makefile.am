AM_CONDITIONAL(PCAPMMAP, test x$pcapmmap = xyes)

# depends on --enable-pcapmmap, what pcap library is used
if PCAPMMAP
	LPCAP=$(top_builddir)/../libpcap-mmap/libpcap.a
else
	LPCAP=-lpcap
endif

SUBDIRS=common cfg concentrator ipfixlolib sampler reconf tools tests

bin_PROGRAMS=vermont

vermont_SOURCES = \
	collector_configuration.cc collector_configuration.h \
	cfg/graph.h \
	common/msg.h \
	dbreader_configuration.cc dbreader_configuration.h \
	dbwriter_configuration.cc dbwriter_configuration.h \
	exporter_configuration.cc exporter_configuration.h \
	flowmetering_configuration.cc flowmetering_configuration.h \
	ipfix_configuration.cc ipfix_configuration.h \
	metering_configuration.cc metering_configuration.h \
	observer_configuration.cc observer_configuration.h \
	packetreporting_configuration.cc packetreporting_configuration.h \
	packetselection_configuration.cc packetselection_configuration.h \
	reconf/Emitable.h \
	reconf/FilterModule.h \
	reconf/Destination.h \
	reconf/Source.h \
	reconf/PacketDestination.h \
	reconf/PacketSource.h \
	reconf/PacketConnectionQueue.h \
	vermont.cc \
	vermontmain_configuration.cc vermontmain_configuration.h

AM_CFLAGS=-I$(top_srcdir) `xml2-config --cflags` $(MYSQL_CFLAGS) -Wall -Werror
AM_CXXFLAGS=$(AM_CFLAGS)
AM_LDFLAGS= \
	-L$(top_builddir)/cfg \
	-L$(top_builddir)/concentrator \
	-L$(top_builddir)/sampler \
	-L$(top_builddir)/ipfixlolib \
	`xml2-config --libs` \
	$(MYSQL_LDFLAGS) \
	$(BOOST_LDFLAGS) \
	$(BOOST_FILESYSTEM_LIB) \
	$(BOOST_REGEX_LIB)

vermont_LDADD= \
	$(top_builddir)/cfg/libcfg.a \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/common/libcommon.a \
	$(top_builddir)/reconf/libreconf.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a \
	-lpthread \
	$(LPCAP)

vermont_DEPENDENCIES= \
	$(top_builddir)/cfg/libcfg.a \
	$(top_builddir)/concentrator/libconcentrator.a \
	$(top_builddir)/sampler/libsampler.a \
	$(top_builddir)/common/libcommon.a \
	$(top_builddir)/reconf/libreconf.a \
	$(top_builddir)/ipfixlolib/libipfixlo.a
