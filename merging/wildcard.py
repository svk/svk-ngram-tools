from sys import stdin

# as of now this generates all variants (2**n, 32).

# almost certainly we can exclude some of them;
# for instance, wildcards at the beginning or
# at the end are not useful.
# that alone (the other possibility would be
# restricting to one contiguous stretch of wcs)
# reduces us to 2**(n-2) meaning max 8.
# this is almost certainly manageable

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
