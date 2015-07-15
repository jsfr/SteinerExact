import json
import glob

def collectData(testSet):
    methods = ['Simple', 'SimpleSort', 'SmithNew', 'SmithNewSort', 'SmithOld']
    allData = {}
    for method in methods:
        pattern = '../%s/%s/*.txt'%(testSet, method)
        data = {}
        for filename in glob.glob(pattern):
            with open(filename) as file:
                res = json.load(file)
            n = res['nodes']
            if n not in data:
                data[n] = {'n': 0, 'times': [], 'iterations': [], 'trees': []}
            data[n]['n'] += 1
            if res['success']:
                data[n]['times'].append(res['time'])
                data[n]['iterations'].append(res['iterations'])
                data[n]['trees'].append(res['trees'])
        allData[method] = data
    return allData

def iterationTable():
    return

def treesTable():
    return

def timesPlot():
    return

data = {}
sets = ['Carioca', 'Iowa05', 'Sausage', 'Solids']
for set in sets:
    data[set] = collectData(set)
