INCLUDES=-I.
CC=gcc
CFLAGS=-O2 -DDEBUG -g $(INCLUDES) -pg
CXXFLAGS=$(CFLAGS)
LIBS=-lstdc++ -lpcap -lpthread
LDFLAGS=$(LIBS) $(CFLAGS)
TARGET=vermont
OBJS=vermont.o subsystems.o msg.o iniparser.o config_sampler.o config_concentrator.o
MODULES=ipfixlolib sampler concentrator
MODULES_LIBS=sampler/libsampler.a ipfixlolib/libipfixlo.a concentrator/libconcentrator.a

all: modules $(TARGET)

modules:
	for dir in $(MODULES); do \
		(cd $$dir; $(MAKE) all); \
	done

	
$(TARGET): $(OBJS) $(MODULES_LIBS)
	g++ $(CFLAGS) -o $(TARGET) $(OBJS) $(MODULES_LIBS) $(LIBS)

clean:
	rm -f *.o $(TARGET)
	for dir in $(MODULES); do \
		(cd $$dir; $(MAKE) clean); \
	done

distclean: clean
	rm -f *~

config_sampler.o: config_sampler.cc ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Packet.h msg.h \
  sampler/Lock.h sampler/Filter.h sampler/ConcurrentQueue.h \
  sampler/Semaphore.h sampler/Thread.h sampler/TimeoutSemaphore.h \
  sampler/PacketReceiver.h sampler/PacketProcessor.h \
  sampler/SystematicSampler.h sampler/RandomSampler.h sampler/Observer.h \
  sampler/PacketSink.h sampler/Sink.h sampler/ExporterSink.h \
  sampler/Template.h sampler/IPHeaderFilter.h vermont.h iniparser.h \
  subsystems.h config_sampler.h
vermont.o: vermont.cc iniparser.h msg.h subsystems.h vermont.h \
  sampler/Template.h sampler/Observer.h sampler/Thread.h \
  sampler/ConcurrentQueue.h sampler/Lock.h sampler/TimeoutSemaphore.h \
  sampler/Semaphore.h sampler/Packet.h sampler/PacketReceiver.h \
  sampler/Filter.h sampler/PacketProcessor.h sampler/SystematicSampler.h \
  sampler/RandomSampler.h sampler/ExporterSink.h ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Sink.h \
  config_sampler.h
iniparser.o: iniparser.c iniparser.h
msg.o: msg.c msg.h
subsystems.o: subsystems.c subsystems.h msg.h
config_concentrator.o: config_concentrator.cc vermont.h iniparser.h \
  sampler/Template.h sampler/Observer.h sampler/Thread.h msg.h \
  sampler/ConcurrentQueue.h sampler/Lock.h sampler/TimeoutSemaphore.h \
  sampler/Semaphore.h sampler/Packet.h sampler/PacketReceiver.h \
  sampler/Filter.h sampler/PacketProcessor.h sampler/SystematicSampler.h \
  sampler/RandomSampler.h sampler/ExporterSink.h ipfixlolib/ipfixlolib.h \
  ipfixlolib/encoding.h ipfixlolib/ipfix_names.h sampler/Sink.h \
  sampler/HookingFilter.h concentrator/rcvIpfix.h \
  concentrator/aggregator.h concentrator/rules.h concentrator/hashing.h \
  concentrator/sndIpfix.h subsystems.h config_concentrator.h
