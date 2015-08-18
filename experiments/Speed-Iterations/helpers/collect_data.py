#!/bin/python

import json
import glob
import numpy
import scipy.stats.mstats

def collectData(pattern):
    ret = {"succ": 0, "fail": 0, "times": {}, "iterations": {}, "trees": {}}
    times = []
    iterations = []
    trees = []
    for filename in glob.glob(pattern):
        with open(filename) as file:
            res = json.load(file)
        if res['success']:
            ret["succ"] += 1
            times.append(res["time"])
            iterations.append(res["iterations"])
            trees.append(res["trees"])
        else:
            ret["fail"] += 1

    if times:
        ret["times"]["arithmean"] = numpy.mean(times)
        ret["times"]["geomean"] = scipy.stats.mstats.gmean(times)
        ret["times"]["std"] = numpy.std(times)
        ret["times"]["median"] = numpy.median(times)
        ret["times"]["results"] = times
    else:
        ret["times"]["arithmean"] = float('NaN')
        ret["times"]["geomean"] = float('NaN')
        ret["times"]["std"] = float('NaN')
        ret["times"]["median"] = float('NaN')
        ret["times"]["times"] = times


    if iterations:
        ret["iterations"]["arithmean"] = numpy.mean(iterations)
        ret["iterations"]["geomean"] = scipy.stats.mstats.gmean(iterations)
        ret["iterations"]["std"] = numpy.std(iterations)
        ret["iterations"]["median"] = numpy.median(iterations)
        ret["iterations"]["results"] = iterations
    else:
        ret["iterations"]["arithmean"] = float('NaN')
        ret["iterations"]["geomean"] = float('NaN')
        ret["iterations"]["std"] = float('NaN')
        ret["iterations"]["median"] = float('NaN')
        ret["iterations"]["iterations"] = iterations

    if trees:
        ret["trees"]["arithmean"] = numpy.mean(trees)
        ret["trees"]["geomean"] = scipy.stats.mstats.gmean(trees)
        ret["trees"]["std"] = numpy.std(trees)
        ret["trees"]["median"] = numpy.median(trees)
        ret["trees"]["results"] = trees
    else:
        ret["trees"]["arithmean"] = float('NaN')
        ret["trees"]["geomean"] = float('NaN')
        ret["trees"]["std"] = float('NaN')
        ret["trees"]["median"] = float('NaN')
        ret["trees"]["trees"] = trees

    return ret

def collectCarioca(methods):
    data = []
    for d in range(3,5):
        for n in range(11,16):
            for method in methods:
                res = collectData("../Carioca/%s/carioca_%d_%d_*.txt"%(method,d,n))
                data.append({"name":"carioca", "d": d, "n": n, "method": method, "results": res})
    return data

def collectCube(methods):
    data = []
    for d in range(2,4):
        for n in range(10,15):
            for method in methods:
                res = collectData("../Cube/%s/cube_n%d_d%d_*.txt"%(method,n,d))
                data.append({"name":"cube", "d": d, "n": n, "method": method, "results": res})
    return data

def collectIowa(methods):
    data = []
    for d in range(3,5):
            for method in methods:
                res = collectData("../Iowa05/%s/inst10x%d_*.txt"%(method,n,d))
                data.append({"name":"iowa", "d": d, "n": 10, "method": method, "results": res})
    return data

def collectSausage(methods):
    data = []
    for d in range(2,5):
        for n in range(10,15):
            for method in methods:
                res = collectData("../Sausage/%s/sausage_n%d_d%d_*.txt"%(method,n,d))
                data.append({"name":"sausage", "d": d, "n": n, "method": method, "results": res})
    return data

def collectSolids(methods):
    data = []
    vertexMap = {'tetrahedron': 4, 'octahedron': 6, 'icosahedron':12, 'dodecahedron': 20, 'cube': 8}
    for solid in vertexMap:
        for method in methods:
            res = collectData("../Solids/%s/%s.txt"%(method,solid))
            data.append({"name":solid, "d": 3, "n": vertexMap[solid], "method": method, "results": res})
    return data

def succFailTable():
    methods = ["SmithOld", "SmithNew", "SmithNewSort", "Simple", "SimpleSort"]
    data = collectCarioca(methods)
    data = collectCube(methods)
    data = collectIowa(method)
    data = collectSausage(methods)
    data = collectSolids(methods)
