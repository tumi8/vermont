#
# Makefile for vermont's tools
#
# This is a gmake (GNU make) Makefile, don't even try pmake (BSD make)
#

ARCH ?= BSD

CFLAGS += -g -pg -Wall -Werror -I../concentrator
LDFLAGS += -g -pg 

ifeq ($(ARCH),BSD)
CFLAGS += -pthread
LDFLAGS += -pthread
else
LDLIBS += -l pthread 
endif

.PHONY: all clean

all: testCollector

#	make -C ipfixlolib	

clean:
	rm -f testCollector testCollector.o gmon.out

testCollector: testCollector.o ../concentrator/libconcentrator.a ../ipfixlolib/libipfixlo.a ../msg.o

../concentrator/libconcentrator.a:
	make -C../concentrator/ libconcentrator.a

../ipfixlolib/libipfixlo.a:
	make -C../ipfixlolib/ libipfixlo.a

../msg.o:
	make -C../ msg.o
