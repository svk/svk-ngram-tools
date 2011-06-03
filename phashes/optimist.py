from hashlib import md5, sha1
from sys import stdin

# done mostly to check what sort of scale we're looking at collisions
# for here.

length = 12

def t(h,w):
    return h(w).hexdigest()[:length]

while True:
    line = stdin.readline()
    if not line:
        break
    word, _ = line.split()
    print word, t(md5,word), t(sha1,word)

