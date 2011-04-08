# Converts a _sorted_ (in whatever manner) sequence of n-grams to
# a B-tree.

from ngrams import NgramReader
from struct import pack

class DirectoryNamer:
    def __init__(self, n, digits): # produces strings of length (n + digits), plus need one more for nul
        assert n < digits
        self.n = n
        self.digits = digits
        self.fmt = "%%0%dd" % self.digits
    def __call__(self, k):
        s = self.fmt % k
        return "/".join( [ s[-(i+1)] for i in range(self.n) ] + [ s[:-self.n] ] )

SimpleFileNamer = lambda n : "%d" % n

UseGzip = False
RecordSize = 256
MaxTokenSize = 232 # longest 5-gram + spaces is 204 (.) (Note we need one more for NUL)
TokenNameTemplate = DirectoryNamer( 2, 4 )
TokenNameLength = 16 # need one more for NUL, for convenience
FirstTokenId = 1
MaxEntriesInNode = 128 # 128-ish is a reasonable value -- may want to tweak so we can cache an appropriate amount

NextTokenId = FirstTokenId
EntryPacker = "=%dsQ%ds" % (MaxTokenSize, TokenNameLength)

assert (MaxTokenSize + 8 + TokenNameLength) == RecordSize

class TreeNode:
    def __init__(self, dirname, openFile):
        self.parent = None
        self.openFile = openFile
        self.dirname = dirname
        self.clear()
    def clear(self):
        global NextTokenId
        self.id = NextTokenId
        NextTokenId = self.id + 1
        self.name = TokenNameTemplate( self.id )
        self.entries = []
    def headerentry(self):
        return self.entries[0][:-1] + (self.name,)
    def write(self):
        from os import path, makedirs
        import errno
        fullname = "%s/%s" % (self.dirname, self.name)
        dirname, _ = path.split( fullname )
        print dirname, "was dirname"
        try:
            makedirs( dirname )
        except OSError as e:
            if e.errno == errno.EEXIST:
                pass
            else:
                raise
        f = self.openFile( fullname, "wb" )
        for entry in self.entries:
            assert( len( self.name ) < TokenNameLength )
            print entry
            rawentry = pack( EntryPacker, *entry )
            assert( len(rawentry) <= RecordSize )
            f.write( rawentry )
        f.close()
    def full(self):
        return len( self.entries ) == MaxEntriesInNode
    def add(self, entry):
        self.entries.append( entry )
        if self.full():
            self.write()
            if not self.parent:
                self.parent = TreeNode( self.dirname, self.openFile )
            self.parent.add( self.headerentry() )
            self.clear()
    def finalize(self):
        if not self.parent:
            self.name = "root"
        self.write()
        if self.parent:
            self.parent.add( self.headerentry() )
            return self.parent.finalize()
        return self

if __name__ == '__main__':
    from sys import argv
    import gzip

    dirname = argv[1]
    files = argv[2:]
    print dirname, files

    node = TreeNode( dirname, gzip.open if UseGzip else open)

    for filename in files:
        f = gzip.open( filename, "rb" )
        try:
            for tokens, count in NgramReader(f):
                entry = (" ".join(tokens), count, "")
                node.add( entry )
        finally:
            f.close()

    root = node.finalize()
    print root.name
