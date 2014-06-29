TARGET=./test
LIBS=-lrt
INCLUDE=
CC?=gcc
CFLAGS:=-std=c11 -Wall -fpic -O3

OBJECTS = $(patsubst %.cpp, %.o, $(wildcard *.cpp)) \
          $(patsubst %.c, %.o, $(wildcard *.c)) 

HEADERS = $(wildcard *.h)
INCLUDECC = $(addprefix -I,$(INCLUDE))

.PHONY: default all clean self_test
.PRECIOUS: $(TARGET) $(OBJECTS)
.SUFFIXES:

default: $(TARGET) $(TARGET).s
all: default

debug: CFLAGS+= -g -ggdb3
debug: default

%.s: %
	objdump -D -S $< > $@

%.o: %.c $(HEADERS)
	$(CC) $(INCLUDECC) $(CFLAGS) -c $< $(LIBS) -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

run: $(TARGET)
	$(TARGET)

self_test:
	$(MAKE) -B run CC=gcc
	$(MAKE) -B run CC=clang

clean:
	-rm -f *.o
	-rm -f $(TARGET) $(TARGET).s
