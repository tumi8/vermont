# Prelim dirty makefile
INCLUDES=-I. -I..
CFLAGS=-O2 -Wall -pg $(INCLUDES)
CXXFLAGS=$(CFLAGS)
CC=gcc
CXX=g++

TARGET=libconcentrator.a
OBJS= aggregator.o crc16.o hashing.o ipfix.o printIpfix.o rcvIpfix.o rules.o sampler_hook_entry.o sndIpfix.o templateBuffer.o

all: $(TARGET)

$(TARGET): $(OBJS)
	ar cru $(TARGET) $(OBJS)
	ranlib $(TARGET)

clean:
	rm -f *.o $(TARGET)

distclean: clean
	rm -f *~
 
aggregator.o: aggregator.c aggregator.h rcvIpfix.h rules.h hashing.h \
  sndIpfix.h ../msg.h
crc16.o: crc16.c crc16.h
hashing.o: hashing.c hashing.h rcvIpfix.h rules.h crc16.h ipfix.h \
  ../msg.h
ipfix.o: ipfix.c ../ipfixlolib/ipfixlolib.h ../ipfixlolib/encoding.h \
  ../ipfixlolib/ipfix_names.h ipfix.h
printIpfix.o: printIpfix.c printIpfix.h rcvIpfix.h
rcvIpfix.o: rcvIpfix.c rcvIpfix.h templateBuffer.h ipfix.h ../msg.h
rules.o: rules.c rules.h rcvIpfix.h ipfix.h ../msg.h
sampler_hook_entry.o: sampler_hook_entry.c aggregator.h rcvIpfix.h \
  rules.h hashing.h ../sampler/packet_hook.h sampler_hook_entry.h \
  ../msg.h
sndIpfix.o: sndIpfix.c sndIpfix.h rcvIpfix.h ../ipfixlolib/ipfixlolib.h \
  ../ipfixlolib/encoding.h ../ipfixlolib/ipfix_names.h ipfix.h ../msg.h
templateBuffer.o: templateBuffer.c templateBuffer.h rcvIpfix.h ipfix.h \
  ../msg.h
