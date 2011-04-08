CFLAGS=-std=c99 -g
CC=gcc
LDFLAGS=-lz

GETOPTDIR=../lib/freegetopt-0.11
BINARIES=test-lookup

all: $(BINARIES)

clean:
	rm -f *.o
	rm -f a.out
	rm -f $(GETOPTDIR)/*.o
	rm -f $(BINARIES)

test-lookup: test-lookup.o btreelu.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^
