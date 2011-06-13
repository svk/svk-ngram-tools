from sys import stdin

wildcard = "<*>"

def variants( xs ):
    rv = []
    if not xs:
        rv.append( () )
    else:
        for suff in variants( xs[1:] ):
            rv.append( (xs[0],) + suff )
            rv.append( (wildcard,) + suff )
    return rv

while True:
    v = stdin.readline()
    if not v:
        break
    key, value = v.strip().split("\t")
    for key in map( lambda xs : " ".join( xs ), variants( key.split() ) ):
        print "%s\t%s" % (key,value)
