#!/bin/python

from collect_data import *

def plot_boxplot_time(data, setName, methods, suffix, dim=2):
    cutoff_time = 12*60*60

    #Convert data
    plotdata = [{'method': m, 'd': dim, 'color':c, 'fill': f} for (m,c,f) in methods]

    for pdict in plotdata:
        #Store times
        tmp = [ (dic['n'], dic['results']['times']['data']) for dic in data if
                dic['method']==pdict['method'] and
                dic['d']==pdict['d'] ]
        tmp.sort(key=lambda x: x[0])
        tmp = [x[1] for x in tmp ]

        pdict['results'] = tmp

        #Store succ's
        tmp = [ (dic['n'], dic['results']['succ']) for dic in data if
                dic['method']==pdict['method'] and
                dic['d']==pdict['d'] ]
        tmp.sort(key=lambda x: x[0])
        tmp = [x[1] for x in tmp ]

        pdict['succ'] = tmp

    import numpy as np
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon

    fig, ax1 = plt.subplots(figsize=(10,6))
    ax1.set_yscale('log')

    #Grid
    ax1.yaxis.grid(True, linestyle='-', which='major', color='lightgrey', alpha=0.5)
    ax1.set_axisbelow(True)


    ns = list(set([dic['n'] for dic in data]))
    ns.sort()

    for pdict in plotdata:

        offset = 0.16 * (plotdata.index(pdict)-(len(plotdata)-1)/2.0)
        pos = [ x+offset for x in ns ]
        bp = plt.boxplot(pdict['results'], widths=0.13, positions = pos )
        plt.setp(bp['boxes'], color=pdict['color'])
        plt.setp(bp['whiskers'], color=pdict['color'], linestyle='solid')
        plt.setp(bp['fliers'], color=pdict['color'], marker='+')
        plt.setp(bp['caps'], color=pdict['color'])

        for i in range(len(pdict['results'])):
            box = bp['boxes'][i]
            boxX = []
            boxY = []
            for j in range(5):
                boxX.append(box.get_xdata()[j])
                boxY.append(box.get_ydata()[j])
            boxCoords = list(zip(boxX,boxY))
            boxPolygon = Polygon(boxCoords, facecolor=pdict['fill'])
            ax1.add_patch(boxPolygon)

        #Draw number of succesfully completed runs
        for n in ns:
            i = ns.index(n)
            ax1.text(pos[i]-0.05, 0.02, str(pdict['succ'][i]), fontsize=10)

    #Succesful label
    ax1.text(min(ns)-1.27, 0.02, "Completed:", fontsize=10)

    #Set tick positions and labels
    ax1.set_xticks(ns)
    ax1.set_xticklabels(ns)

    #Horizontal line at cutoff time (12hours)
    plt.plot([0,20],[cutoff_time,cutoff_time], color='#a93030', linestyle='dashed' )

    #Hack to add legend
    handles = []
    labels  = []
    methodNames = [x[0] for x in methods]
    p, = plt.plot([1,1], linestyle='dashed', color='#a93030')
    handles.append(p)
    labels.append("Stop time")
    for pdict in plotdata:
        p, = plt.plot([1,1],linewidth=8.0, linestyle='-', color=pdict['fill'])
        handles.append(p)
        labels.append(methodNames[plotdata.index(pdict)])
    plt.legend( handles, labels, loc='upper left' )
    for p in handles:
        p.set_visible(False)

    #Axis labels
    plt.xlabel("Number of terminals")
    plt.ylabel("CPU-time (s)")

    #Title
    plt.title(setName+" (d = "+str(dim)+")")

    plt.xlim(min(ns)-1.3, max(ns)+0.7)
    plt.ylim(0.01,100000)
    plt.savefig("/home/jens/repos/masters-report/report/gfx/boxplots/plot_nvst_boxplot_d"+str(dim)+"_"+setName+"_"+suffix+".pdf")

def plot_boxplot_time_2(data, setName, methods, suffix, terminals=10):
    cutoff_time = 12*60*60

    #Convert data
    plotdata = [{'method': m, 'n': terminals, 'color':c, 'fill': f} for (m,c,f) in methods]

    for pdict in plotdata:
        #Store times
        tmp = [ (dic['d'], dic['results']['times']['data']) for dic in data if
                dic['method']==pdict['method'] and
                dic['n']==pdict['n'] ]
        tmp.sort(key=lambda x: x[0])
        tmp = [x[1] for x in tmp ]

        pdict['results'] = tmp

        #Store succ's
        tmp = [ (dic['d'], dic['results']['succ']) for dic in data if
                dic['method']==pdict['method'] and
                dic['n']==pdict['n'] ]
        tmp.sort(key=lambda x: x[0])
        tmp = [x[1] for x in tmp ]

        pdict['succ'] = tmp

    import numpy as np
    import matplotlib.pyplot as plt
    from matplotlib.patches import Polygon

    fig, ax1 = plt.subplots(figsize=(10,6))
    ax1.set_yscale('log')

    #Grid
    ax1.yaxis.grid(True, linestyle='-', which='major', color='lightgrey', alpha=0.5)
    ax1.set_axisbelow(True)


    ns = list(set([dic['d'] for dic in data]))
    ns.sort()

    for pdict in plotdata:

        offset = 0.16 * (plotdata.index(pdict)-(len(plotdata)-1)/2.0)
        pos = [ x+offset for x in ns ]
        bp = plt.boxplot(pdict['results'], widths=0.13, positions = pos )
        plt.setp(bp['boxes'], color=pdict['color'])
        plt.setp(bp['whiskers'], color=pdict['color'], linestyle='solid')
        plt.setp(bp['fliers'], color=pdict['color'], marker='+')
        plt.setp(bp['caps'], color=pdict['color'])

        for i in range(len(pdict['results'])):
            box = bp['boxes'][i]
            boxX = []
            boxY = []
            for j in range(5):
                boxX.append(box.get_xdata()[j])
                boxY.append(box.get_ydata()[j])
            boxCoords = list(zip(boxX,boxY))
            boxPolygon = Polygon(boxCoords, facecolor=pdict['fill'])
            ax1.add_patch(boxPolygon)

        #Draw number of succesfully completed runs
        for n in ns:
            i = ns.index(n)
            ax1.text(pos[i]-0.05, 0.02, str(pdict['succ'][i]), fontsize=10)

    #Succesful label
    ax1.text(min(ns)-1.27, 0.02, "Completed:", fontsize=10)

    #Set tick positions and labels
    ax1.set_xticks(ns)
    ax1.set_xticklabels(ns)

    #Horizontal line at cutoff time (12hours)
    plt.plot([0,20],[cutoff_time,cutoff_time], color='#a93030', linestyle='dashed' )

    #Hack to add legend
    handles = []
    labels  = []
    methodNames = [x[0] for x in methods]
    p, = plt.plot([1,1], linestyle='dashed', color='#a93030')
    handles.append(p)
    labels.append("Stop time")
    for pdict in plotdata:
        p, = plt.plot([1,1],linewidth=8.0, linestyle='-', color=pdict['fill'])
        handles.append(p)
        labels.append(methodNames[plotdata.index(pdict)])
    plt.legend( handles, labels, loc='upper left' )
    for p in handles:
        p.set_visible(False)

    #Axis labels
    plt.xlabel("Number of dimensions")
    plt.ylabel("CPU-time (s)")

    #Title
    plt.title(setName+" (n = "+str(terminals)+")")

    plt.xlim(min(ns)-1.3, max(ns)+0.7)
    plt.ylim(0.01,100000)
    plt.savefig("/home/jens/repos/masters-report/report/gfx/boxplots/plot_nvst_boxplot_n"+str(terminals)+"_"+setName+"_"+suffix+".pdf")

methods = [
    ("SmithOld",     "#004400", "#55aa55"),
    ("SmithNew",     "#061539", "#46628e"),
    ("SmithNewSort", "#460024", "#af5784"),
    ("Simple",       "#550200", "#d46d6a"),
    ("SimpleSort",   "#554600", "#d4c26a")
]

data = collectCube([x[0] for x in methods])
for d in range(3,5):
    plot = plot_boxplot_time(data, "Cube", methods, "1", dim=d)

data = collectCarioca([x[0] for x in methods])
for d in range(3,5):
    plot = plot_boxplot_time(data, "Carioca", methods, "1", dim=d)

data = collectIowa([x[0] for x in methods])
plot = plot_boxplot_time_2(data, "Iowa", methods, "1", terminals=10)
