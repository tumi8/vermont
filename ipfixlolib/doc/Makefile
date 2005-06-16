CC=gcc
INCLUDES=-I../
# default flags:
#CFLAGS=-O -Wall $(INCLUDES)

# flags for debugging:
CFLAGS=-g -O0 -Wall

TARGET=example_code

all: $(TARGET)

clean:
	rm -f $(TARGET) *.o

example_code: example_code.c
#	gcc -I ../src -o  example_code example_code.c ../src/ipfixlolib.o ../src/encoding.o
	$(MAKE) -C .. all
	$(MAKE) -C ../.. msg.o
	$(CC) $(INCLUDES) $(CFLAGS) -o $(TARGET)  example_code.c ../ipfixlolib.o ../encoding.o ../../msg.o
