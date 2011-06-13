from sys import stdin, stderr, argv

files = list( map( lambda fn : open(fn, "r"), argv[1:] ) )
bufs = [ None for i in range(len(files)) ]

while files:
    for i in range(len(files)):
        if not bufs[i]:
            lines = files[i].readlines(1000000) # arg is bytes
            if not lines:
                files[i].close()
                files[i] = None
            else:
                bufs[i] = list( map( lambda line : line.strip().split("\t"), lines ) )
    files = [ f for f in files if f ]
    bufs = [ b for b in bufs if b ]
    if not files:
        break
#    j = min( range(len(bufs)), key = lambda i : bufs[i][0][0] ) # GAH not present in old Python
    j = 0
    best = None
    for k in range(len(bufs)):
        if (best == None) or (bufs[j][0][0] < best):
            j = k
            best = bufs[k][0][0]
    key, value = bufs[j].pop(0)
    print "%s\t%s" % (key,value)
