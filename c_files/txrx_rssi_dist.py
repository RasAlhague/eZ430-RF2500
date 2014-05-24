#!/usr/bin/env python
import sys
if __name__ == '__main__':
    char = ''
    line = ''
    while 1:
	char = sys.stdin.read(1)
        if char == '\n':
	    sline = line.split()
	    sumsum = 0
	    counter = 0
	    for i in range(len(sline)):
		try:
			sline[i] = int(sline[i])
			sumsum += sline[i]
			counter += 1
		except:
			sline[i] = 0
	    result = sumsum/counter
	    print result
	    line = ''
        else:
	    line = line + char
