from sys import stdin, stderr, argv

files = list( map( lambda fn : open(fn, "r"), argv[1:] ) )
bufs = [ 0 for i in range(len(files)) ]
q = []

while files:
    for i in [ i for i in range(len(files)) if bufs[i] <= 0 ]:
        lines = files[i].readlines(1000000) # arg is bytes
        if not lines:
            files[i].close()
            files[i] = None
        else:
            for line in lines:
                k, v = line.strip().split("\t")
                heappush( q, (k,v,i) )
                bufs[i] += 1
    files = [ f for f in files if f ]
    bufs = [ b for b in bufs if b > 0 ]
    if not files:
        break
    k, v, j = heappop(q)
    bufs[j] -= 1
    print "%s\t%s\n" % (k,v)
