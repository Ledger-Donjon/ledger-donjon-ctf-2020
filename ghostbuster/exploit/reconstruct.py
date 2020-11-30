#!/usr/bin/env python3

"""
Parse ./monitor output to display the secret in line.

  $ ./monitor |& ./reconstruct.py
"""

import os
import re
import sys

def print_secret(secret):
    s = ''
    for d in secret:
        c = max(d, key = lambda k: d[k])
        s += c
    print(f"[{s}]")

if __name__ == '__main__':
    secret = []
    for i in range(0, 32):
        secret.append(dict({".": 0}))

    skip = [ " ", ",", "K", ]

    print_secret(secret)
    while True:
        line = sys.stdin.readline()

        # [ 1] O 79 47
        m = re.match("\[ *(\d+)\] ([^ ]) (\d+) (\d+)", line)
        if not m:
            print(f"invalid input ({line})")
            continue

        index = int(m.group(1))
        c = chr(int(m.group(3)))

        # skip this value
        if c in skip:
            continue

        # update scores
        if not c in secret[index]:
            secret[index][c] = 1
        else:
            secret[index][c] += 1

        print_secret(secret)
