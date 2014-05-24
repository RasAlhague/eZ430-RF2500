#!/usr/bin/env python
import re, Gnuplot, sys
if __name__ == '__main__':
    print 'display started...'
    g = Gnuplot.Gnuplot(debug=1)
    g('set term x11 title "eZWSN sprectum analyzer"')
    g('set yrange [-110:0]')
    g('set data style linespoints')
    g.xlabel('frequency (MHz)')
    g.ylabel('RSSI (dBm)')
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
		frequency = 2433.0+(0.2*i)
                data.append([frequency,sline[i]])
	    g.plot(data)
	    line = ''
        else:
	    line = line + char
