#!/usr/bin/python3
#Created by Stash Kempinski (0843845) 
import sys
import numpy


receivedByAP = []
successBySensor = []
droppedBySensor = []
retriesNeeded = []

cameraPacketsTimes = dict()

for i in range(0,50):
	cameraPacketsTimes[i] = []

#outputted = open("720", "r", errors="ignore") #Ignore errors related to encoding 
#lines = outputted.readlines()
#outputted.close()
lines = sys.stdin.readlines()


for line in lines:
	lineClean = line.replace("\n", "")

	splitted = lineClean.split(" ")

	if len(splitted) < 2:
		continue

	if splitted[0][0] == '[':
		if splitted[2] == 'Received':
			newSplit = splitted[1].split(":")
			cameraDec = int(newSplit[3], 16)
			cameraPacketsTimes[cameraDec].append(int(splitted[0][1:-1]))

	if splitted[1] == 'sent:':
		retriesNeeded.append(int(splitted[6][-2]))


timeTaken = []

for cp in cameraPacketsTimes:
	if cameraPacketsTimes[cp]:
		minn = min(cameraPacketsTimes[cp])
		maxx = max(cameraPacketsTimes[cp])
		
		timeTaken.append(maxx - minn)
		print(f"{minn} {maxx}")

if len(timeTaken) > 0:
	print(sum(timeTaken) / len(timeTaken))
else:
	print(0.0)
#print(numpy.setdiff1d(droppedBySensor, receivedByAP))
if len(retriesNeeded) > 0:
	print(sum(retriesNeeded) / len(retriesNeeded))
else:
	print(0.0)
