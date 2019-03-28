import sys
import numpy as np
import os
import math
import matplotlib.pyplot as plt
import argparse

def scale(valueSets) :
  maxVal = float("-inf")
  for vSet in valueSets :
    maxV = np.amax(vSet)
    if (maxV > maxVal) :
      maxVal = maxV

  for vSet in valueSets :
    vSet = vSet / maxVal

parser = argparse.ArgumentParser()
parser.add_argument("dataFolder")
parser.add_argument("dumpPeriod", help="How many generations per data set", type=int)
parser.add_argument("maxGens", help="Number of generations in experiment", type=int)

args = parser.parse_args()

folder = os.getcwd() + "/"
OUT_PATH = folder + "output/"

gens = range(0, args.maxGens + 1, args.dumpPeriod)
print(gens)

fig = 0
dataX = []
dataY = []
dataCol = []

# Collect data
for gen in gens :
  dataFile = folder + args.dataFolder + "/archive_" + str(gen) + ".dat"

  dataX.append([])
  dataY.append([])
  dataCol.append([])
  
  with open(dataFile) as f :
    for l in f :
      data = l.split()
      dataX[fig].append(float(data[1]))
      dataY[fig].append(float(data[2]))
      dataCol[fig].append(float(data[3]))
  dataCol[fig] = np.array(dataCol[fig])
  dataCol[fig] = np.abs(dataCol[fig])
  fig += 1
  
fig = 0
# Scale output together (colour scaled by absolute variance)
for gen in gens :
  scale(dataCol)

  plt.figure(fig)
  plt.title("Generation " + str(gen) + " Output")
  plt.scatter(dataX[fig], dataY[fig], c=dataCol[fig], s=20)
  plt.axis([0.0, 1.0, 0.0, 1.0])
  plt.gray()
  fig += 1

plt.show()
