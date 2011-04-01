CFLAGS=-std=c99
CC=gcc

GETOPTDIR=./freegetopt-0.11
BINARIES=btprint test-bins

all: $(BINARIES)

clean:
	rm -f *.o
	rm -f a.out
	rm -f $(GETOPTDIR)/*.o
	rm -f $(BINARIES)

btprint: btprint.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(CFLAGS) -o $@ $^

test-bins: bins_test.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(CFLAGS) -o $@ $^
