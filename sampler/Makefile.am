bin_PROGRAMS = sampler


# set the include path found by configure
INCLUDES = $(all_includes)

# the library search path.
sampler_LDFLAGS = $(all_libraries) 
sampler_SOURCES = Observer.cpp PacketSink.cpp Test.cpp SystematicSampler.cpp Filter.cpp ConfigManager.cpp RandomSampler.cpp Template.cpp Packet.cpp IPHeaderFilter.cpp ExporterSink.cpp
sampler_LDADD = -lpcap -lpthread -lipfixlo -L../../ipfixlolib/src

AM_CFLAGS = -DDEBUG -I../../ipfixlolib/src
AM_CXXFLAGS = -DDEBUG -I../../ipfixlolib/src
noinst_HEADERS = PacketProcessor.h Thread.h Globals.h Lock.h Observer.h Packet.h PacketSink.h Semaphore.h ConcurrentQueue.h SystematicSampler.h Filter.h ConfigManager.h TimeoutLock.h RandomSampler.h ConfigFile.h Template.h IPHeaderFilter.h IPHeaderFilter.h ExporterSink.h
