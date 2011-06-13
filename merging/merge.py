from sys import stdin, stderr

lastKey, lastValue = None, None

def dump():
    print "%s\t%d" % (lastKey, lastValue)

while True:
    vs = stdin.readlines(1000000)
    if not vs:
        break
    for v in vs:
        if not v:
            break
        key, value = v.strip().split("\t")
        if key == lastKey:
            lastValue += int(value)
        elif lastKey:
            dump()
            lastKey, lastValue = key, int(value)
        else:
            lastKey, lastValue = key, int(value)
dump()
