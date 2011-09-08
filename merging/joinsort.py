from sys import stdin, stderr, argv
from heapq import heappush, heappop

files = list( map( lambda fn : [open(fn, "r"), 0], argv[1:] ) )
q = []
empty = list(files)

while files:
    while empty:
        f = empty.pop(0)
        lines = f[0].readlines(1000000) # arg is bytes
        if not lines:
            f[0].close()
            f[0] = None
        else:
            for line in lines:
                k, v = line.strip().split("\t")
                heappush( q, (k,v,f) )
                f[1] += 1
    if not q:
        break
    k, v, f = heappop(q)
    f[1] -= 1
    if f[1] <= 0:
        empty.append( f )
    print "%s\t%s" % (k,v)
