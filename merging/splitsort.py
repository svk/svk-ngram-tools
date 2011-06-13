from sys import stdin, argv

prefix = argv[1]
buffersize = 4000000 # lines

buf = []
bufs = 0
bufno = 1

def flush():
    global buf, bufs, bufno
    fn = "%s.sorted.%08d" % (prefix, bufno)
    bufno += 1
    buf.sort( key = lambda t : t[0] )
    f = open( fn, "w" )
    for k,v in buf:
        print>>f, "%s\t%s" % (k,v)
    f.close()
    buf = []
    bufs = 0
    

while True:
    line = stdin.readline()
    if not line:
        break
    buf.append( line.strip().split( "\t" ) )
    bufs += 1
    if bufs >= buffersize:
        flush()
if buf:
    flush()
