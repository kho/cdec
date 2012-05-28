#!/usr/bin/env python

import re
import sys

for (lineno, line) in enumerate(sys.stdin, 1):
    parts = map(str.strip, line.replace(' ||| ||| ', ' |||  ||| ').split(' ||| '))
    if len(parts) != 4:
        raise Exception('%d parts found in line %d: %s' % (len(parts), lineno, line.strip()))
    print ' ||| '.join([parts[0], parts[1], parts[3]])
