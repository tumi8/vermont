CC=gcc
CFLAGS=-O -Wall -lpthread
INCLUDES=-I.. -I../..

TARGET=init_deinit template_generation identifier_naming

all: $(TARGET)

clean:
	rm -f $(TARGET) *.o

testrun:
# loop over all targets and run them
# not there yet
	echo "not implemented target"

init_deinit: init_deinit.c
	$(CC) $(INCLUDES) $(CFLAGS) -o init_deinit init_deinit.c ../libipfixlo.a ../../msg.o

template_generation: template_generation.c
	$(CC) $(INCLUDES) $(CFLAGS)  -o template_generation template_generation.c ../libipfixlo.a ../../msg.o

identifier_naming: identifier_naming.c
	$(CC) $(INCLUDES) $(CFLAGS)  -o identifier_naming identifier_naming.c ../libipfixlo.a ../../msg.o
