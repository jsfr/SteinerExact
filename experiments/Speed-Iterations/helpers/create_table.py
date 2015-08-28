#!/bin/python

from collect_data import *
import numpy

def print_table_sausage(table_type):
    methods = ["Simple", "SimpleSort", "SmithNew", "SmithNewSort", "SmithOld"]
    data = collectSausage(methods)

    print(r"\begin{tabular}{ccccccc}")
    print(r"\toprule")
    print(r"& & \multicolumn{5}{c}{Method} \\")
    print(r"\cmidrule(l){3-7}")
    print(r"$n$ & $d$ & Simple & SimpleSort & SmithNew & SmithNewSort & SmithOld \\")
    print(r"\cmidrule(r){1-2}\cmidrule(l){3-7}")

    for n in range(10,16):
        print("$"+str(n)+"$", end='')
        for d in range(2,6):
            old_val = [x["results"][table_type]["mean"] for x in data
                       if x["d"] == d and x["n"] == n and x["method"] == "SmithOld"][0]
            values = []
            for m in methods:
                new_val = [x["results"][table_type]["mean"] for x in data
                           if x["d"] == d and x["n"] == n and x["method"] == m][0]
                val = new_val/old_val
                val = ("$%.2f$"%(val) if not numpy.isnan(val) else " ")
                values.append(val)
            print(" & $" + str(d) + "$ & " + ' & '.join(values) + r" \\")
        if n < 15:
            print(r"\cmidrule(r){1-2}")
    print(r"\bottomrule")
    print(r"\end{tabular}")

def print_table_solids(table_type):
    methods = ["Simple", "SimpleSort", "SmithNew", "SmithNewSort", "SmithOld"]
    data = collectSolids(methods)

    print(r"\begin{tabular}{cccccc}")
    print(r"\toprule")
    print(r"& \multicolumn{5}{c}{Method} \\")
    print(r"\cmidrule(l){2-6}")
    print(r"$n$ & Simple & SimpleSort & SmithNew & SmithNewSort & SmithOld \\")
    print(r"\cmidrule(r){1-1}\cmidrule(l){2-6}")

    d = 3

    for n in [4,6,8,12,20]:
        print("$"+str(n)+"$", end='')
        old_val = [x["results"][table_type]["mean"] for x in data
                   if x["d"] == d and x["n"] == n and x["method"] == "SmithOld"][0]
        values = []
        for m in methods:
            new_val = [x["results"][table_type]["mean"] for x in data
                       if x["d"] == d and x["n"] == n and x["method"] == m][0]
            val = new_val/old_val
            val = ("$%.2f$"%(val) if not numpy.isnan(val) else " ")
            values.append(val)
        print(" & $" + str(d) + "$ & " + ' & '.join(values) + r" \\")
    print(r"\bottomrule")
    print(r"\end{tabular}")

def print_table_cube(table_type):
    methods = ["Simple", "SimpleSort", "SmithNew", "SmithNewSort", "SmithOld"]
    data = collectCube(methods)

    print(r"\begin{tabular}{ccccccc}")
    print(r"\toprule")
    print(r"& & \multicolumn{5}{c}{Method} \\")
    print(r"\cmidrule(l){3-7}")
    print(r"$n$ & $d$ & Simple & SimpleSort & SmithNew & SmithNewSort & SmithOld \\")
    print(r"\cmidrule(r){1-2}\cmidrule(l){3-7}")

    for n in range(10,16):
        print("$"+str(n)+"$", end='')
        for d in range(2,5):
            old_val = [x["results"][table_type]["mean"] for x in data
                       if x["d"] == d and x["n"] == n and x["method"] == "SmithOld"][0]
            values = []
            for m in methods:
                new_val = [x["results"][table_type]["mean"] for x in data
                           if x["d"] == d and x["n"] == n and x["method"] == m][0]
                val = new_val/old_val
                val = ("$%.2f$"%(val) if not numpy.isnan(val) else " ")
                values.append(val)
            print(" & $" + str(d) + "$ & " + ' & '.join(values) + r" \\")
        if n < 15:
            print(r"\cmidrule(r){1-2}")
    print(r"\bottomrule")
    print(r"\end{tabular}")


print_table_cube("trees")
print_table_cube("iterations")
print_table_cube("times")
