SUBDIRS=concentrator ipfixlolib sampler tools

bin_PROGRAMS=vermont

vermont_SOURCES=collector_configuration.cc collector_configuration.h \
exporter_configuration.cc exporter_configuration.h metering_configuration.cc \
metering_configuration.h msg.c msg.h observer_configuration.cc \
observer_configuration.h vermont.cc ipfix_configuration.cc \
ipfix_configuration.h vermontmain_configuration.h vermontmain_configuration.cc\
packetselection_configuration.h packetselection_configuration.cc \
packetreporting_configuration.h packetreporting_configuration.cc \
flowmetering_configuration.h flowmetering_configuration.cc \
dbwriter_configuration.h dbwriter_configuration.cc \
dbreader_configuration.h dbreader_configuration.cc \
expressflowmetering_configuration.h expressflowmetering_configuration.cc

AM_CFLAGS=-I$(top_srcdir) `xml2-config --cflags` $(MYSQL_CFLAGS) -Wall -Werror -DDEBUG
AM_CXXFLAGS=$(AM_CFLAGS) 
AM_LDFLAGS=-L$(top_builddir)/concentrator -L$(top_builddir)/sampler -L$(top_builddir)/ipfixlolib `xml2-config --libs` $(MYSQL_LDFLAGS) -lboost_regex
vermont_LDADD=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/sampler/libsampler.a $(top_builddir)/ipfixlolib/libipfixlo.a -lpcap -lpthread
vermont_DEPENDENCIES=$(top_builddir)/concentrator/libconcentrator.a $(top_builddir)/sampler/libsampler.a $(top_builddir)/ipfixlolib/libipfixlo.a
