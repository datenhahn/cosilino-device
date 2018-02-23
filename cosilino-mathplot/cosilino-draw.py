#!/usr/bin/env python

import gtk
from matplotlib.backends.backend_gtkagg import FigureCanvasGTKAgg as FigureCanvas
from matplotlib.backends.backend_gtkagg import NavigationToolbar2GTKAgg as NavigationToolbar
from matplotlib.figure import Figure

import datetime
from pymongo import MongoClient

client = MongoClient('your-raspberrypi', 27017)

# Get the sampleDB database
db = client.cosilino
stats = db['statistics']

win = gtk.Window()
win.connect("destroy", lambda x: gtk.main_quit())
win.set_default_size(400, 300)
win.set_title("Embedding in GTK")

vbox = gtk.VBox()
win.add(vbox)

fig = Figure(figsize=(5, 4), dpi=100)
ax = fig.add_subplot(111)

graph = [entry['roomTemp'] for entry in stats.find() if entry['date'] > datetime.datetime.fromtimestamp(1519084800)]
xaxis = [entry['date'] for entry in stats.find() if entry['date'] > datetime.datetime.fromtimestamp(1519084800)]
graph2 = [entry['roomHum'] for entry in stats.find() if entry['date'] > datetime.datetime.fromtimestamp(1519084800)]
graph3 = [entry['heaterTemp'] for entry in stats.find() if entry['date'] > datetime.datetime.fromtimestamp(1519084800)]
graph4 = [round(entry['heaterPower']/10)+30 for entry in stats.find() if entry['date'] > datetime.datetime.fromtimestamp(1519084800)]

ax.plot(xaxis, graph, label="Room Temp")
ax.plot(xaxis, graph2, label="Room Hum")
ax.plot(xaxis, graph3, label="Heater Temp")
ax.plot(xaxis, graph4, label="Heater Power (/10 + 30)")

legend = ax.legend(loc='upper center', shadow=True)

canvas = FigureCanvas(fig)  # a gtk.DrawingArea
vbox.pack_start(canvas)
toolbar = NavigationToolbar(canvas, win)
vbox.pack_start(toolbar, False, False)

win.show_all()
gtk.main()
