#!/usr/bin/env python
import re, Gnuplot, sys
if __name__ == '__main__':
    print 'display started...'
    g = Gnuplot.Gnuplot(debug=1)
    g('set term x11 title "gradient"')
    g('set yrange [-20:40]')
    g('set data style linespoints')
    g.xlabel('depth')
    g.ylabel('temperature (C)')
    char = ''
    line = ''
    while 1:
	char = sys.stdin.read(1)
        if char == '\n':
	    sline = line.split()
	    data = []
	    for i in range(len(sline)):
		try:
			sline[i] = int(sline[i])
		except:
			sline[i] = 0
		temperature = (((sline[i]/1023.0)*1500)-500)/10/256
                data.append([i*10,temperature])
	    g.plot(data)
	    line = ''
        else:
	    line = line + char
