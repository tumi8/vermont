INCLUDES=-I.
CC=gcc
CFLAGS=-O -DDEBUG -pg $(INCLUDES)
CXXFLAGS=$(CFLAGS)
LIBS=-lstdc++ -lrt -lpcap
LDFLAGS=$(LIBS) $(CFLAGS)
TARGET=vermont
OBJS=vermont.o subsystems.o msg.o iniparser.o config_sampler.o

$(TARGET): $(OBJS)
	g++ $(CFLAGS) -o $(TARGET) $(OBJS) sampler/*.o ipfixlolib/libipfixlo.a $(LIBS)

clean:
	rm -f *.o $(TARGET)
distclean: clean
	rm -f *~

config_sampler.o: config_sampler.cc ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Packet.h msg.h \
  sampler/Lock.h sampler/Filter.h sampler/ConcurrentQueue.h \
  sampler/TimeoutLock.h sampler/Semaphore.h sampler/Thread.h \
  sampler/PacketReceiver.h sampler/PacketProcessor.h \
  sampler/SystematicSampler.h sampler/RandomSampler.h sampler/Observer.h \
  sampler/PacketSink.h sampler/Sink.h sampler/ExporterSink.h \
  sampler/Template.h sampler/IPHeaderFilter.h vermont.h iniparser.h \
  subsystems.h config_sampler.h
vermont.o: vermont.cc iniparser.h msg.h subsystems.h vermont.h \
  sampler/Template.h sampler/Observer.h sampler/Thread.h \
  sampler/ConcurrentQueue.h sampler/Lock.h sampler/TimeoutLock.h \
  sampler/Semaphore.h sampler/Packet.h sampler/PacketReceiver.h \
  sampler/Filter.h sampler/PacketProcessor.h sampler/SystematicSampler.h \
  sampler/RandomSampler.h sampler/ExporterSink.h ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Sink.h \
  config_sampler.h
iniparser.o: iniparser.c iniparser.h
msg.o: msg.c msg.h
subsystems.o: subsystems.c subsystems.h msg.h
