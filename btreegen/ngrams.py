# n-grams must be in proper format -- lines separated
# with newlines, n-gram from count by a single tab character,
# tokens by a single space character, compressed with
# gzip. (This is the format of the Google files.)

class NgramReader:
    def __init__(self, f):
        self.f = f
    def __iter__(self):
        it = iter( self.f )
        class MyIter:
            def __iter__(iself):
                return iself
            def next(iself):
                stokens, scount = it.next().strip().split("\t")
                return (tuple(stokens.split()), int(scount))
        return MyIter()

if __name__ == '__main__':
    import gzip
    f = gzip.open( "../data/5gm-0112.gz" )
    maxlen = 0
    try:
        for tokens, count in NgramReader(f):
            l = len( " ".join( tokens ) )
            if l > maxlen:
                maxlen = l
                print tokens, l
    finally:
        print maxlen
        f.close()
