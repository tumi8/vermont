INCLUDES=-Iipfixlolib/ -Isampler/ -Icollector/
CC=gcc
CFLAGS=-O $(INCLUDES)
TARGET=vermont

$(TARGET): vermont.o subsystems.o msg.o iniparser.o

clean:
	rm -f *.o
distclean: clean
	rm -f *~


vermont.o: vermont.cc
subsystems.o: subsystems.c subsystems.h msg.h
msg.o: msg.c msg.h
iniparser.o: iniparser.c iniparser.h
