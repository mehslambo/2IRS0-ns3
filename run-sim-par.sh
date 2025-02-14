#!/bin/bash

#
#  Run multiple NS-3 simulations in parallel to
#  investigate different combinations of parameters.
#
#  Usage:
#
#     run-sim-par.sh  N
#
#  where N is the number of parallel NS-3 instances.
#
#  Before running this script, compile ns-3 tools using waf.
#  This script directly calls the compiled simulation tools
#  and doesn't use waf at that stage.
#

# initialize a semaphore with a given number of tokens
open_sem() {
    mkfifo pipe-$$
    exec 3<>pipe-$$
    rm pipe-$$
    local i=$1
    for ((;i>0;i--)); do
	printf '%s' 000 >&3
    done
}

# run the given command asynchronously and pop/push tokens
run_with_lock() {
    local x
    # with error handling:
    #read -u 3 -n 3 x && ((0==x)) || exit $x
    # without error handling:
    read -u 3 -n 3 x
    (
	("$@"; )
	printf '%.3d' $? >&3
    ) &
}

open_sem $1

mkdir -p ../output/halow

export LD_LIBRARY_PATH=$PWD/build:$LD_LIBRARY_PATH


for perAxis in 4 5 6 7 8 9 10; do
   ((sensors=perAxis*perAxis))
   for packetSize in 1 5 10 15 20 25; do
      echo "Sensors=$sensors,  packet=$packetSize" 
      for dist in 10 15 20 25 30 40 50 75 100; do
	echo -n "$dist "  
	for depth in 0.2 0.3 0.4 0.5 0.6 0.7; do
	    run_with_lock build/scratch/sewer/sewer --sensors=$perAxis --packetSize=$packetSize --totalSize=$packetSize --dist=$dist --depth=$depth --pageSliceCount=0 --NRawSta=60 --NGroup=6 --RAWConfigFile=./OptimalRawGroup/RawConfig-60-6-2.txt >../output/halow/halow-sewer-60s6-$perAxis-$packetSize-$packetSize-$dist-$depth.txt 2>&1
        done
      done
      echo " done."
      for areaSize in 100 200 300 400 500 600 700 800 900 1000; do
	echo -n "$areaSize"
        for depth in 0.2 0.3 0.4; do
           run_with_lock build/scratch/soil/soil --sensors=$sensors --perAxis=$perAxis --packetSize=$packetSize --totalSize=$packetSize --areaSize=$areaSize --depth=$depth --pageSliceCount=0 --NRawSta=120 --NGroup=8 --RAWConfigFile=./OptimalRawGroup/RawConfig-120-8-3.txt >../output/halow/halow-soil-120s8-$sensors-$packetSize-$packetSize-$areaSize-$depth.txt 2>&1
        done
	run_with_lock build/scratch/bins/bins --sensors=$sensors --perAxis=$perAxis --packetSize=$packetSize --totalSize=$packetSize --areaSize=$areaSize --pageSliceCount=0 --NRawSta=120 --NGroup=8 --RAWConfigFile=./OptimalRawGroup/RawConfig-120-8-3.txt >../output/halow/halow-bins-120s8-$sensors-$packetSize-$packetSize-$areaSize.txt 2>&1
        run_with_lock build/scratch/bins/bins --sensors=$sensors --perAxis=$perAxis --packetSize=$packetSize --totalSize=$packetSize --areaSize=$areaSize --pageSliceCount=0 --NRawSta=120 --NGroup=6 --RAWConfigFile=./OptimalRawGroup/RawConfig-120-6-4.txt >../output/halow/halow-bins-120s6-$sensors-$packetSize-$packetSize-$areaSize.txt 2>&1
        run_with_lock build/scratch/bins/bins --sensors=$sensors --perAxis=$perAxis --packetSize=$packetSize --totalSize=$packetSize --areaSize=$areaSize --pageSliceCount=0 --NRawSta=120 --NGroup=5 --RAWConfigFile=./OptimalRawGroup/RawConfig-120-5-4.txt >../output/halow/halow-bins-120s5-$sensors-$packetSize-$packetSize-$areaSize.txt 2>&1
        run_with_lock build/scratch/bins/bins --sensors=$sensors --perAxis=$perAxis --packetSize=$packetSize --totalSize=$packetSize --areaSize=$areaSize --pageSliceCount=0 --NRawSta=120 --NGroup=4 --RAWConfigFile=./OptimalRawGroup/RawConfig-120-4-6.txt >../output/halow/halow-bins-120s4-$sensors-$packetSize-$packetSize-$areaSize.txt 2>&1
        run_with_lock build/scratch/bins/bins --sensors=$sensors --perAxis=$perAxis --packetSize=$packetSize --totalSize=$packetSize --areaSize=$areaSize --pageSliceCount=0 --NRawSta=120 --NGroup=3 --RAWConfigFile=./OptimalRawGroup/RawConfig-120-3-8.txt >../output/halow/halow-bins-120s3-$sensors-$packetSize-$packetSize-$areaSize.txt 2>&1
      done
      echo " done."
   done
done

for posts in 4 8 12; do
    for camerasPerPost in 2 3 4; do
      echo "posts=$posts, cameras=$camemasPerPost";
      for packetSize in 1500 2000; do
         for totalSize in 3760 10440 17640 31360 94090; do
	    run_with_lock build/scratch/trespass/trespass --posts=$posts --camerasPerPost=$camerasPerPost --packetSize=$packetSize --totalSize=$totalSize --pageSliceCount=0 --NRawSta=60 --NGroup=6 --RAWConfigFile=./OptimalRawGroup/RawConfig-60-6-2.txt >../output/halow/halow-trespass-60s6-$posts-$camerasPerPost-$packetSize-$totalSize.txt 2>&1
         done
      done
   done
done

wait
