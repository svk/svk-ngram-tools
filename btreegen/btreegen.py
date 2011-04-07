# Converts a _sorted_ (in whatever manner) sequence of n-grams to
# a B-tree.

from ngrams import NgramReader
from struct import pack

RecordSize = 256
MaxTokenSize = 240 # longest 5-gram + spaces is 204 (?)
TokenNameTemplate = "%07d"
TokenNameMaxLength = 8
FirstTokenId = 1
MaxEntriesInNode = 128 # low for testing, 128-ish is a reasonable value

NextTokenId = FirstTokenId
EntryPacker = "=%dsQ%ds" % (MaxTokenSize, TokenNameMaxLength)

assert (MaxTokenSize + 8 + TokenNameMaxLength) == RecordSize

class TreeNode:
    def __init__(self, openFile):
        global NextTokenId
        self.id = NextTokenId
        NextTokenId = self.id + 1
        self.name = TokenNameTemplate % self.id
        self.parent = None
        self.entries = []
        self.openFile = openFile
    def clear(self):
        global NextTokenId
        self.id = NextTokenId
        NextTokenId = self.id + 1
        self.name = TokenNameTemplate % self.id
        self.entries = []
    def headerentry(self):
        return self.entries[0][:-1] + (self.name,)
    def write(self):
        f = self.openFile( self.name )
        for entry in self.entries:
            assert( len( self.name ) < TokenNameMaxLength )
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
                self.parent = TreeNode( self.openFile )
            self.parent.add( self.headerentry() )
            self.clear()
    def finalize(self):
        self.write()
        if self.parent:
            self.parent.add( self.headerentry() )
            return self.parent.finalize()
        return self

if __name__ == '__main__':
    from sys import argv
    import gzip

    dir = argv[1]
    files = argv[2:]
    print dir, files

    node = TreeNode( lambda s : gzip.open( "%s/%s" % (dir,s), "wb" )  )

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
