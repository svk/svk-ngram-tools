from sys import stdin, argv

prefix = argv[1]
buffersize = 10000000 # lines

f = None
bufno = 1
written = 0

def nextfile():
    global f, bufno, written
    if f:
        f.close()
    fn = "%s.%08d" % (prefix, bufno)
    bufno += 1
    written = 0
    f = open( fn, "w" )

nextfile()

def w(s):
    global f, written
    print >> f, s,
    written += 1
    if written >= buffersize:
        nextfile()

while True:
    lines = stdin.readlines(1000000)
    if not lines:
        break
    for line in lines:
        w(line)
if f:
    f.close()
