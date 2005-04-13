INCLUDES=-I.
CC=gcc
CFLAGS=-O $(INCLUDES)
CXXFLAGS=$(CFLAGS)
LIBS=-lstdc++ -lrt -lpcap
LDFLAGS=$(LIBS)
TARGET=vermont

$(TARGET): vermont.o subsystems.o msg.o iniparser.o config_sampler.o

clean:
	rm -f *.o
distclean: clean
	rm -f *~


config_sampler.o: config_sampler.cc ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Packet.h \
  sampler/Globals.h sampler/Lock.h sampler/Filter.h \
  sampler/ConcurrentQueue.h sampler/TimeoutLock.h sampler/Semaphore.h \
  sampler/Thread.h sampler/PacketReceiver.h sampler/PacketProcessor.h \
  sampler/SystematicSampler.h sampler/RandomSampler.h sampler/Observer.h \
  sampler/PacketSink.h sampler/ExporterSink.h sampler/Template.h \
  sampler/IPHeaderFilter.h vermont.h iniparser.h msg.h subsystems.h \
  config_sampler.h
vermont.o: vermont.cc ipfixlolib/ipfixlolib.h ipfixlolib/encoding.h \
  ipfixlolib/ipfix_names.h iniparser.h msg.h subsystems.h vermont.h \
  sampler/Template.h sampler/Globals.h sampler/Observer.h \
  sampler/Thread.h sampler/ConcurrentQueue.h sampler/Lock.h \
  sampler/TimeoutLock.h sampler/Semaphore.h sampler/Packet.h \
  sampler/PacketReceiver.h sampler/Filter.h sampler/PacketProcessor.h \
  sampler/SystematicSampler.h sampler/RandomSampler.h \
  sampler/ExporterSink.h config_sampler.h
iniparser.o: iniparser.c iniparser.h
msg.o: msg.c msg.h
subsystems.o: subsystems.c subsystems.h msg.h

