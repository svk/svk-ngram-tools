from sys import stdin
from transform import transformation
from random import randint

# a simple script to make test queries from n-gram files
# lines are prefixed with numbers to make scrambling easier
# no longer needed if we can run the tests with a transforming
# dictionary

while True:
    line = stdin.readline()
    if not line:
        break
    key, value = line.strip().split("\t")
    keys = key.split()
    tkey = " ".join( map( transformation, keys ) )
    print randint(1,2**30), tkey
