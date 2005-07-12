# Prelim dirty makefile
INCLUDES=-I..
CFLAGS=-O2 $(INCLUDES) -Wall
CXXFLAGS=$(CFLAGS)
CC=gcc
CXX=g++

TARGET=libsampler.a
OBJS=ExporterSink.o Filter.o HookingFilter.o IPHeaderFilter.o Observer.o Packet.o PacketSink.o RandomSampler.o SystematicSampler.o Template.o 

all: $(TARGET)


clean:
	rm -f *.o $(TARGET)
distclean: clean
	rm -f *~

$(TARGET): $(OBJS)
	ar cru $(TARGET) $(OBJS)
	ranlib $(TARGET)

ExporterSink.o: ExporterSink.cpp ../msg.h ../ipfixlolib/ipfixlolib.h \
  ../ipfixlolib/encoding.h ../ipfixlolib/ipfix_names.h ExporterSink.h \
  Thread.h Template.h Packet.h Lock.h Sink.h PacketReceiver.h \
  ConcurrentQueue.h TimeoutSemaphore.h
Filter.o: Filter.cpp ../msg.h Filter.h ConcurrentQueue.h Lock.h \
  TimeoutSemaphore.h Packet.h Thread.h PacketReceiver.h PacketProcessor.h \
  SystematicSampler.h RandomSampler.h
HookingFilter.o: HookingFilter.cpp HookingFilter.h PacketProcessor.h \
  Packet.h ../msg.h Lock.h packet_hook.h
IPHeaderFilter.o: IPHeaderFilter.cpp IPHeaderFilter.h ../msg.h \
  PacketProcessor.h Packet.h Lock.h
Observer.o: Observer.cpp ../msg.h Observer.h Thread.h ConcurrentQueue.h \
  Lock.h TimeoutSemaphore.h Packet.h PacketReceiver.h
Packet.o: Packet.cpp Packet.h ../msg.h Lock.h
PacketSink.o: PacketSink.cpp ../msg.h PacketSink.h Thread.h Sink.h \
  PacketReceiver.h ConcurrentQueue.h Lock.h TimeoutSemaphore.h Packet.h
RandomSampler.o: RandomSampler.cpp RandomSampler.h ../msg.h \
  PacketProcessor.h Packet.h Lock.h
SystematicSampler.o: SystematicSampler.cpp SystematicSampler.h \
  PacketProcessor.h Packet.h ../msg.h Lock.h
Template.o: Template.cpp ../msg.h Template.h Packet.h Lock.h
