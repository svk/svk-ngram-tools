# A script to create lists of queries for performance-testing purposes.
# This script generates tests in a _fairly_ logical way (not something
#  arbitrary like using the first parts of each file):
#       - One pass is made through each n-gram. Each n-gram is included
#         with a certain probability
#       - Convert randomly to wildcards according to a certain probability
#         distribution (or never do this)

from __future__ import with_statement

def read_ngrams( args, counts = False ):
    import gzip
    for arg in args:
        f = gzip.open( arg, "r" )
        line = f.readline()
        while line:
            elements = line.split("\t")
            if counts:
                yield elements[0], int(elements[-1])
            else:
                yield elements[0]
            line = f.readline()
        f.close()

def count_entries( args ):
    rv = 0
    for ngram in read_ngrams( args ):
        rv += 1
    return rv


def process_entries( out, accept, args, wildcarder, progress, transformer, test_target = None ):
    rv = 0
    processed = 0
    for ngram in read_ngrams( args ):
        ngram = ngram.split(" ")
        from sys import stderr
        if accept():
            out( wildcarder( map( transformer, ngram ) ) )
            rv += 1
            if test_target and rv >= test_target:
                break
        processed += 1
        progress( processed )
    return rv


if __name__ == '__main__':
    from optparse import OptionParser
    parser = OptionParser()
    parser.add_option( "-o", "--output", dest="filename",
                       help="write queries to FILE", metavar="FILE")
    parser.add_option( "-c", "--count", dest="count",
                       help="skip counting pass, there are NUM entries", metavar="NUM",
                       default = None )
    parser.add_option( "-1", "--count-1grams", action="store_true", dest="count_1grams", 
                       help="skip counting pass, use standard count for all the 1-grams (may be combined with the others)",
                       default = False )
    parser.add_option( "-2", "--count-2grams", action="store_true", dest="count_2grams", 
                       help="skip counting pass, use standard count for all the 2-grams (may be combined with the others)",
                       default = False )
    parser.add_option( "-3", "--count-3grams", action="store_true", dest="count_3grams", 
                       help="skip counting pass, use standard count for all the 3-grams (may be combined with the others)",
                       default = False )
    parser.add_option( "-4", "--count-4grams", action="store_true", dest="count_4grams", 
                       help="skip counting pass, use standard count for all the 4-grams (may be combined with the others)",
                       default = False )
    parser.add_option( "-5", "--count-5grams", action="store_true", dest="count_5grams", 
                       help="skip counting pass, use standard count for all the 5-grams (may be combined with the others)",
                       default = False )
    parser.add_option( "-q", "--queries", dest="queries_to_generate",
                       help="generate approximately NUM queries", metavar="NUM",
                       default = 1000000)
    parser.add_option( "-s", "--seed", dest="seed",
                       help="use NUM as random seed", metavar="NUM",
                       default = None )
    parser.add_option( "-r", "--random-shuffle", action="store_true", dest="shuffle",
                       help="shuffle the queries (keeping them in memory until the very end!)", metavar="NUM",
                       default = False )
    parser.add_option( "-w", "--wildcard-independent", dest = "wildcard_independent",
                       help="generate with NUM independent probability of every token becoming a wildcard", metavar="NUM",
                       default = None )
    parser.add_option( "-T", "--transformer", dest = "transformer",
                       help="preprocess using transformer", metavar="NUM",
                       default = None )
    parser.add_option( "-t", "--test-mode", dest = "test_mode", action="store_true", default = False,
                       help="test mode (stop after reaching goal, greatly increase probability)", metavar="NUM" )
    options, args = parser.parse_args()
    StandardCounts = {
        1: 13588391,
        2: 314843401,
        3: 977069902,
        4: 1313818354,
        5: 1176470663
    }
    from random import Random
    from sys import stdout, stderr
    from time import time
    r = Random()
    out = stdout
    if options.seed:
        r = Random( options.seed )
    if options.filename:
        out = open( options.filename, "w" )
    if options.count:
        count = options.count
    elif options.count_1grams or options.count_2grams or options.count_3grams or options.count_4grams or options.count_5grams:
        count = 0
        if options.count_1grams:
            count += StandardCounts[1]
        if options.count_2grams:
            count += StandardCounts[2]
        if options.count_3grams:
            count += StandardCounts[3]
        if options.count_4grams:
            count += StandardCounts[4]
        if options.count_5grams:
            count += StandardCounts[5]
        print >> stderr, "Counting %d n-grams." % count
    else:
        t0 = time()
        count = count_entries( args )
        t1 = time()
        print >> stderr, "Counted %d n-grams (took %0.2lf seconds)." % (count, t1-t0)
    wildcarder = lambda x : x
    if options.wildcard_independent:
        chance = float( options.wildcard_independent )
        def f( xs ):
            rv = []
            for x in xs:
                if r.random() < chance:
                    rv.append( "<*>" )
                else:
                    rv.append( x )
            return tuple( rv )
        wildcarder = f
    transform = lambda x : x
    if options.transformer:
        d = {}
        print >> stderr, "Loading transformer from %s.." % options.transformer,
        import gzip
        try:
            f = gzip.open( options.transformer, "r" )
            for line in f:
                key, value = line.split( "\t" )
                d[ key ] = int( value )
        finally:
            f.close()
        print >> stderr, "done."
        transform = lambda x : d[x]
    def print_ngram( ngram ):
        s = " ".join( wildcarder( ngram ) )
        print >> out, s
    output = print_ngram
    collection = []
    if options.shuffle:
        def add_ngram_to_collection( x ):
            collection.append( x )
        output = add_ngram_to_collection
    target = int(options.queries_to_generate)
    test_target = None
    probability = target / float( count )
    if options.test_mode:
        print >> stderr, "Running in TEST MODE (will generate low-quality results quickly)."
        test_target = target
        probability = min( 1.0, probability * 1000 )
        probability = 1.0
    print >> stderr, "Running with probability %lf." % probability
    t0 = time()
    def progress( n ):
        N = count
        n0 = (n*100)//N
        nn1 = ((n-1)*100)//N
        if n > 0 and n0 != nn1:
            elapsed = time() - t0
            speed = n / elapsed
            remaining = (N-n) / speed
            print "%d%% done, %0.lf seconds remaining.." % (n0, remaining)
    generated = process_entries( output, lambda : r.random() < probability, args, wildcarder, progress, transformer = transform, test_target = test_target)
    if options.shuffle:
        print >> stderr, "Generation done, shuffling and writing.."
        r.shuffle( collection )
        for ngram in collection:
            print_ngram( ngram )
    t1 = time()
    print >> stderr, "Generated %d queries (%0.2lf of target, took %0.2lf seconds)." % (generated, (generated/float(target)*100.0), t1 - t0)
