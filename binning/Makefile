CFLAGS=-std=c99 -g
CC=gcc
LDFLAGS=-lz
CPPC=g++

GETOPTDIR=../lib/freegetopt-0.11
BINARIES=btprint test-bins test-ngread mkbins test-binsize showbin

all: $(BINARIES)

clean:
	rm -f *.o
	rm -f a.out
	rm -f $(GETOPTDIR)/*.o
	rm -f $(BINARIES)

showbin: showbin.o bins.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

hashbins-to-sfbt: hashbins-to-sfbt.o hashbin.o semifile.o sfbt.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

sfbt-hashbins-lookup: sfbt-hashbins-lookup.o hashbin.o semifile.o sfbt.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

btprint: btprint.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-bins: test-bins.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-binsize: test-binsize.o bins.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-hash-binsize: test-hash-binsize.o sha1.o bins.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-ngread: test-ngread.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

mkbins: mkbins.o ngread.o bins.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-wordhash: test-wordhash.o ngread.o wordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^
