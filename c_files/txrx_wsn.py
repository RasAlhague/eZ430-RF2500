#!/usr/bin/env python
import re, Gnuplot, sys, random, time
if __name__ == '__main__':
	print 'display started...'
	threshold_remove_node = 50
	g = Gnuplot.Gnuplot(debug=1)
	char = ''
	line = ''
	nodes_id            = []
	nodes_sink          = []
	nodes_coord_x       = []
	nodes_coord_y       = []
	nodes_temperature   = []
	nodes_voltage       = []
	nodes_seqnum        = []
	nodes_timelastheard = []
	link_start          = []
	link_end            = []
	link_rssi           = []
	while 1:
		char = sys.stdin.read(1)
		if char == '\n':
			#######################retrieve the data
			items1 = line.split()
			receiver    = items1[0]
			sender      = items1[1]
			seqnum      = items1[2]
			temperature = items1[3]
			voltage     = items1[4]
			rssi        = items1[5]
			items2=items1[6].split(':')
			items3=items2[1].split(',')
			neighbor_id   = []
			neighbor_rssi = []
			for i in range(len(items3)):
				items4 = items3[i].split('@')
				neighbor_id.append(items4[0])
				neighbor_rssi.append(items4[1])
			items2=items1[7].split(':')
			path=items2[1].split(',')
			#######################populate the tables
			if nodes_id.count(receiver)==0:                         #add receiver to "nodes"
				nodes_id.append(receiver)
				nodes_sink.append(0)
				nodes_coord_x.append(random.randint(0,100))
				nodes_coord_y.append(random.randint(0,100))
				nodes_temperature.append("?")
				nodes_voltage.append("?")
				nodes_seqnum.append("?")
				nodes_timelastheard.append(time.time())
			if nodes_id.count(sender)==0:                           #add sender to "nodes"
				nodes_id.append(sender)
				nodes_sink.append(0)
				nodes_coord_x.append(random.randint(0,100))
				nodes_coord_y.append(random.randint(0,100))
				nodes_temperature.append(temperature)
				nodes_voltage.append(voltage)
				nodes_seqnum.append(seqnum)
				nodes_timelastheard.append(time.time())
			else:                                                   #update data of already added "nodes"
				nodes_temperature[nodes_id.index(sender)] = temperature
				nodes_voltage[nodes_id.index(sender)] = voltage
				nodes_seqnum[nodes_id.index(sender)] = seqnum
				nodes_timelastheard[nodes_id.index(sender)] = time.time()
			for i in range(len(neighbor_id)):                       #add neighbor nodes to "nodes"
				if nodes_id.count(neighbor_id[i])==0:
					nodes_id.append(neighbor_id[i])
					nodes_sink.append(0)
					nodes_coord_x.append(random.randint(0,100))
					nodes_coord_y.append(random.randint(0,100))
					nodes_temperature.append("?")
				        nodes_voltage.append("?")
					nodes_seqnum.append("?")
					nodes_timelastheard.append(time.time())
			for i in range(len(path)):
				if nodes_id.count(path[i])==0:                  #add nodes on route to "nodes"
					nodes_id.append(path[i])
					nodes_sink.append(0)
					nodes_coord_x.append(random.randint(0,100))
					nodes_coord_y.append(random.randint(0,100))
					nodes_temperature.append("?")
				        nodes_voltage.append("?")
					nodes_seqnum.append("?")
					nodes_timelastheard.append(time.time())
			index = 0
			maximum = len(link_start)
			while index < maximum:                                  #remove all sender's neighbors from "links"
				if link_start[index] == sender or link_end[index] == sender:
					link_start.pop(index)
					link_end.pop(index)
					link_rssi.pop(index)
					index=index-1
					maximum = maximum-1
				index=index+1
			index_path = 0                                          #remove hops on the path from "links"
			maximum_path = len(path)
			while index_path < maximum_path-1:
				index = 0
				maximum = len(link_start)
				while index < maximum:
					if (link_start[index] == path[index_path] and link_end[index] == path[index_path+1]) or (link_start[index] == path[index_path+1] and link_end[index] == path[index_path]):
						link_start.pop(index)
						link_end.pop(index)
						link_rssi.pop(index)
						index=index-1
						maximum = maximum-1
					index=index+1
				index_path=index_path+1
			for i in range(len(neighbor_id)):                       #add all neighbors from sender
				link_start.append(sender)
				link_end.append(neighbor_id[i])
				link_rssi.append(neighbor_rssi[i])
			proceed = 1                                             #add last link with RSSI in "links"
			for i in range(len(link_start)):
				if (link_start[i] == path[len(path)-2] and link_end[i] == receiver) or (link_start[i] == receiver and link_end[i] == path[len(path)-2]):
					proceed = 0
					break
			if proceed == 1:
				link_start.append(path[len(path)-2])
				link_end.append(receiver)
				link_rssi.append(rssi)
			#declare last node on path as sink
			nodes_sink[nodes_id.index(path[len(path)-1])] = 1
			#######################remove old information
			for i in range(len(nodes_id)):                          #remove links
				if ((time.time()-nodes_timelastheard[i]>threshold_remove_node) and
				(nodes_sink[i]==0)):
					index = 0
					maximum = len(link_start)
					while index < maximum:
						if (link_start[index]==nodes_id[i] or link_end[index]==nodes_id[i]):
							link_start.pop(index)
							link_end.pop(index)
							link_rssi.pop(index)
							index = index - 1
							maximum = maximum - 1
						index = index + 1
			index = 0                                               #remove nodes
			maximum = len(nodes_id)
			while index < maximum:
				if ((time.time()-nodes_timelastheard[index]>threshold_remove_node) and (nodes_sink[index]==0)):
					nodes_id.pop(index)
					nodes_sink.pop(index)
					nodes_coord_x.pop(index)
					nodes_coord_y.pop(index)
					nodes_temperature.pop(index)
					nodes_voltage.pop(index)
					nodes_seqnum.pop(index)
					nodes_timelastheard.pop(index)
					index = index - 1
					maximum = maximum - 1
				index = index + 1
                        #######################plot
			g('reset')
			g('set term x11 title "eZWSN network visualizer"')
			g('set xrange [0:100]')
			g('set yrange [0:100]')
			g('set format x ""')
			g('set format y ""')
			g('set noxtics')
			g('set noytics')
			g('set noborder')
			g('set pm3d')
			g('set cbrange [-110:-30]')
			g('set palette model RGB defined (-110 \"red\",  -90 \"orange\", -70 \"yellow\", -50 \"green\", -30 \"dark-green\")')
			g('set size square')
			g('set data style points')
			for i in range(len(link_start)):
				string = "set arrow from %s,%s to %s,%s nohead lw 2 linecolor palette cb %f" % (nodes_coord_x[nodes_id.index(link_start[i])],
				                                                                                      nodes_coord_y[nodes_id.index(link_start[i])],
														      nodes_coord_x[nodes_id.index(link_end[i])],
														      nodes_coord_y[nodes_id.index(link_end[i])],
														      int(link_rssi[i]))
				g(string)
			for i in range(len(nodes_id)):
				if (nodes_sink[i]==0):
					string = "set label \"%s,%sV,seq=%s\" at %s,%s" % (nodes_id[i], nodes_voltage[i], nodes_seqnum[i], nodes_coord_x[i], nodes_coord_y[i])
				else:
					string = "set label \"%s\" at %s,%s textcolor ls 1" % (nodes_id[i], nodes_coord_x[i], nodes_coord_y[i])
				g(string)
			data = []
			data.append([-1,-1])
			g.plot(data)
			line = ''
		else:
			line = line + char
