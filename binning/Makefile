TITAN_CFLAGS=-I/ltg/steinavk/lib/include
TITAN_LDFLAGS=-L/ltg/steinavk/lib/lib -Wl,-R/ltg/steinavk/lib/lib
CFLAGS=-std=c99 -g $(TITAN_CFLAGS) $(OPTIMIZE_FLAGS) $(USE_ASSEMBLY_FLAGS)
ASFLAGS=$(USE_ASSEMBLY_FLAGS)
CPPFLAGS=-g $(TITAN_CFLAGS) $(OPTIMIZE_FLAGS) $(USE_ASSEMBLY_FLAGS)
CC=gcc
LDFLAGS=-lz -lJudy $(TITAN_LDFLAGS)
CPPC=g++

GETOPTDIR=../lib/freegetopt-0.11
BINARIES=test-ngread test-judysort judysort-prepare-tapes mergetapes-to-sfbtin sfbtin-hashbins-lookup test-wordhash-j test-mergetapes

RAW_VOCABULARY=../data/vocab.gz
TRANSFORMER=../working/transformer.gz
PROCESSED_VOCABULARY=../working/vocabulary.gz
TEST_5GRAMS=../data/5gm-0015.gz
TEMP_TAPES=./temp-tapes/
TEST_TRANSFORMED_INPUT=../data/transformed.txt
TREE_DIR=../working/trees/
TEMP_DIR=../../tmp/
NGRAM_DIR=../../ngrams/data/

JUDYSORT_ARGS=-L 100000000 -T $(TRANSFORMER) -W

all: $(BINARIES)

clean:
	rm -f *.o
	rm -f a.out
	rm -f $(GETOPTDIR)/*.o
	rm -f $(BINARIES)

$(TEMP_DIR)/tapes-2gram: judysort-prepare-tapes $(TRANSFORMER)
	mkdir -p $@
	zcat $(NGRAM_DIR)2gms/*.gz | ./judysort-prepare-tapes $(JUDYSORT_ARGS) -n 2 -P $(TEMP_DIR)/tapes-2gram/

$(TEMP_DIR)/tapes-3gram: judysort-prepare-tapes $(TRANSFORMER)
	mkdir -p $@
	zcat $(NGRAM_DIR)3gms/*.gz | ./judysort-prepare-tapes $(JUDYSORT_ARGS) -n 3 -P $(TEMP_DIR)/tapes-3gram/

$(TEMP_DIR)/tapes-4gram: judysort-prepare-tapes $(TRANSFORMER)
	mkdir -p $@
	zcat $(NGRAM_DIR)4gms/*.gz | ./judysort-prepare-tapes $(JUDYSORT_ARGS) -n 4 -P $(TEMP_DIR)/tapes-4gram/

$(TEMP_DIR)/tapes-5gram: judysort-prepare-tapes $(TRANSFORMER)
	mkdir -p $@
	zcat $(NGRAM_DIR)5gms/*.gz | ./judysort-prepare-tapes $(JUDYSORT_ARGS) -n 5 -P $(TEMP_DIR)/tapes-5gram/

tree-2gram: mergetapes-to-sfbtin
	./mergetapes-to-sfbtin -c -F -n 2 -B 20 -p 1 -o $(TREE_DIR)/2gram $(TEMP_DIR)/tapes-2gram/*.gz

tree-3gram: mergetapes-to-sfbtin
	./mergetapes-to-sfbtin -c -F -n 3 -B 10 -p 1 -o $(TREE_DIR)/3gram $(TEMP_DIR)/tapes-3gram/*.gz

tree-4gram: mergetapes-to-sfbtin
	./mergetapes-to-sfbtin -c -F -n 4 -B 7 -p 2 -o $(TREE_DIR)/4gram $(TEMP_DIR)/tapes-4gram/*.gz

tree-5gram: mergetapes-to-sfbtin
	./mergetapes-to-sfbtin -c -F -n 5 -B 5 -p 2 -o $(TREE_DIR)/5gram $(TEMP_DIR)/tapes-5gram/*.gz

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

test-hash-binsize: test-hash-binsize.o sha1.o bins.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-ngread: test-ngread.o ngread.o $(GETOPTDIR)/getopt.o
	$(CC) $(LDFLAGS) $(CFLAGS) -o $@ $^

test-wordhash-j: test-wordhash-c.o ngread.o jwordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^

sfbtin-hashbins-lookup: sfbtin-hashbins-lookup.o jwordhash.o hashbin.o semifile.o sfbti.o ngread.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

test-judysort: test-judysort.o judysort.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

judysort-prepare-tapes: judysort-prepare-tapes.o judysort.o $(GETOPTDIR)/getopt.o jwordhash.o ngread.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

test-mergetapes: test-mergetapes.o mergetapes.o mergetapes_asm.o
	$(CPPC) $(LDFLAGS) $(CPPFLAGS) -o $@ $^

mergetapes-to-sfbtin: mergetapes-to-sfbtin.o mergetapes.o mergetapes_asm.o hashbin.o semifile.o sfbti.o ngread.o jwordhash.o $(GETOPTDIR)/getopt.o
	$(CPPC) $(LDFLAGS) $(CFLAGS) -o $@ $^
