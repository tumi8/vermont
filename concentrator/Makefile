#
# Makefile for the IPFIX concentrator project
#
# This is a gmake (GNU make) Makefile, don't even try pmake (BSD make)
#

ARCH ?= BSD

CFLAGS += -g -pg -Wall -Werror
LDFLAGS += -g -pg 

ifeq ($(ARCH),BSD)
CFLAGS += -pthread
LDFLAGS += -pthread
else
LDLIBS += -l pthread 
endif

TARGETS = testCollector testAggregator ipfixCollector.a

.PHONY: all clean

all: $(TARGETS)

build/%.o: src/%.c src/%.h
	$(CC) $(CFLAGS) -o $@ -c $<

clean:
	rm -f $(TARGETS) gmon.out build/*

testAggregator: build/testAggregator.o build/aggregator.o build/rules.o build/common.o build/ipfix.o build/crc16.o build/hashing.o build/sndIpfix.o build/printIpfix.o ipfixCollector.a ipfixlolib/libipfixlo.a
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

testCollector: build/testCollector.o build/printIpfix.o build/common.o ipfixCollector.a
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

ipfixCollector.a: ipfixCollector.a(build/rcvIpfix.o) ipfixCollector.a(build/templateBuffer.o) ipfixCollector.a(build/common.o) ipfixCollector.a(build/ipfix.o)
	ranlib $@
