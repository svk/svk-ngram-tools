TITAN_CFLAGS=-I/ltg/steinavk/lib/include
TITAN_LDFLAGS=-L/ltg/steinavk/lib/lib -Wl,-R/ltg/steinavk/lib/lib
CFLAGS=-std=c99 -g $(TITAN_CFLAGS)
CPPFLAGS=-g $(TITAN_CFLAGS)
CC=gcc
LDFLAGS=-lz -lJudy $(TITAN_LDFLAGS)
CPPC=g++

GETOPTDIR=../lib/freegetopt-0.11
BINARIES=btprint test-bins test-ngread mkbins test-binsize showbin hashbins-to-sfbt hashbins-to-sfbti sfbti-hashbins-lookup sfbt-hashbins-lookup test-judysort judysort-prepare-tapes mergetapes-to-sfbtin

RAW_VOCABULARY=../data/vocab.gz
TRANSFORMER=../working/transformer.gz
PROCESSED_VOCABULARY=../working/vocabulary.gz
TEST_5GRAMS=../data/5gm-0015.gz
TEMP_TAPES=./temp-tapes/
TEST_TRANSFORMED_INPUT=../data/transformed.txt

all: $(BINARIES)

clean:
	rm -f *.o
	rm -f a.out
	rm -f $(GETOPTDIR)/*.o
	rm -f $(BINARIES)

$(TRANSFORMER): ../merging/transform.py ../merging/transform-vocab-unsorted.py $(RAW_VOCABULARY)
	python ../merging/transform-vocab-unsorted.py $(RAW_VOCABULARY) $(TRANSFORMER) $(PROCESSED_VOCABULARY)

$(PROCESSED_VOCABULARY): ../merging/transform.py ../merging/transform-vocab-unsorted.py $(RAW_VOCABULARY)
	python ../merging/transform-vocab-unsorted.py $(RAW_VOCABULARY) $(TRANSFORMER) $(PROCESSED_VOCABULARY)

test-sfbtin.output: $(TRANSFORMER) judysort-prepare-tapes mergetapes-to-sfbtin
	rm -rf $(TEMP_TAPES)
	rm -rf test_sfbtin.output
	mkdir $(TEMP_TAPES)
	zcat $(TEST_5GRAMS) | ./judysort-prepare-tapes -T $(TRANSFORMER) -W -n 5 -P $(TEMP_TAPES) -L 20000000
	./mergetapes-to-sfbtin -c -B 5 -o test-sfbtin.output -n 5 -p 2 $(TEMP_TAPES)/*

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

test-wordhash: test-wordhash.o ngread.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-wordhash-c: test-wordhash-c.o ngread.o wordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-wordhash-j: test-wordhash-c.o ngread.o jwordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^

hashbins-to-sfbti: hashbins-to-sfbti.o hashbin.o semifile.o sfbti.o ngread.o wordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^

sfbti-hashbins-lookup: sfbti-hashbins-lookup.o wordhash.o hashbin.o semifile.o sfbti.o ngread.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

sfbtin-hashbins-lookup: sfbtin-hashbins-lookup.o wordhash.o hashbin.o semifile.o sfbti.o ngread.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

test-judysort: test-judysort.o judysort.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

judysort-prepare-tapes: judysort-prepare-tapes.o judysort.o $(GETOPTDIR)/getopt.o wordhash.o ngread.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

test-mergetapes: test-mergetapes.o mergetapes.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

mergetapes-to-sfbtin: mergetapes-to-sfbtin.o mergetapes.o hashbin.o semifile.o sfbti.o ngread.o wordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^
