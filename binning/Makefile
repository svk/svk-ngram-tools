CFLAGS=-std=c99
CC=gcc
LDFLAGS=-lz

GETOPTDIR=./freegetopt-0.11
BINARIES=btprint test-bins test-ngread mkbins test-binsize

all: $(BINARIES)

clean:
	rm -f *.o
	rm -f a.out
	rm -f $(GETOPTDIR)/*.o
	rm -f $(BINARIES)

btprint: btprint.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-bins: test-bins.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-binsize: test-binsize.o bins.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-ngread: test-ngread.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

mkbins: mkbins.o ngread.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

