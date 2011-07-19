from transform import transformation

next_seen = 0
seen = {}
next_to_write = None

def wordno( word ):
    global seen
    global next_seen
    try:
        return seen[ word ]
    except KeyError:
        rv = seen[ word ] = next_seen
        next_seen += 1
        return rv

def addmapping( key, value, f ):
    global did_add
    did_add = True
    print >>f, "%s\t%d" % (key,value)

def addnewmapping( key, f, g ):
    i = wordno( key ) 
    addmapping( key, i, f )
    print >>g, "%s\t%d" % (key, i)

if __name__ == '__main__':
    from sys import argv, stderr
    from gzip import GzipFile
    raw_vocab = argv[1]
    out_trans = argv[2]
    out_vocab = argv[3]
    fi = GzipFile( raw_vocab, "r" )
    f = GzipFile( out_trans, "w" )
    g = GzipFile( out_vocab, "w" )
    addnewmapping( "<*>", f, g )
    addnewmapping( "<PP_UNK>", f, g )
    next_to_write = next_seen
    p = 0
    while True:
        v = fi.readline()
        if not v:
            break
        key, value = v.strip().split( "\t" )
        nkey = transformation( key )
        nkeyid = wordno( nkey )
        addmapping( key, nkeyid, f )
        if nkeyid >= next_to_write:
            print >>g, "%s\t%d" % (nkey, nkeyid)
            next_to_write = nkeyid + 1
        p += 1
        if (p%100000) == 0:
            print >>stderr, "added", p, key, nkey, nkeyid
    fi.close()
    f.close()
    g.close()
