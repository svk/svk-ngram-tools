from sys import stdin, stderr, argv
from heapq import heappush, heappop

files = list( map( lambda fn : [open(fn, "r"), 0], argv[1:] ) )
q = []

while files:
    for i in [ i for i in range(len(files)) if files[i][0] != None and files[i][1] <= 0 ]:
        lines = files[i][0].readlines(1000000) # arg is bytes
        if not lines:
            files[i][0].close()
            files[i][0] = None
        else:
            for line in lines:
                k, v = line.strip().split("\t")
                heappush( q, (k,v,files[i]) )
                files[i][1] += 1
    if not q:
        break
    k, v, f = heappop(q)
    f[1] -= 1
    print "%s\t%s\n" % (k,v)
