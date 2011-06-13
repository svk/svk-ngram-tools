from sys import stdin

lines = map( lambda t : t.strip().split("\t"), stdin.readlines() )
lines.sort( key = lambda t : t[0] )
for k,v in lines:
    print "%s\t%s" % (k,v)
