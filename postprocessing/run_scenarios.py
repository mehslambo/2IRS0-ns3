#!/usr/bin/env python3
# Runs a set of scenarios in parallel, with different parameters
import os
import subprocess
from concurrent.futures import ProcessPoolExecutor, as_completed
from tqdm import tqdm
import numpy as np

scenarioName = "unified"
argsMatrix = {
    "sensors" : [1],
    "nodeSpacing" : [1],
    "nodeXOffset" : list(np.arange(0.0, 1.5, 0.01)) + list(np.arange(1.5, 1.9, 0.003)) + list(np.arange(1.9, 2.0, 0.01)),
    "propagationModel" : ["air", "freshwater"],
    "stopTime" : [4], # seconds
    "packetSize" : [4096],  # I recommend > 150 bytes to help with filtering ACK/handshakes out
    #"totalSize" : [512]
}

scenarioCommands = [f'{scenarioName}']
for arg, val in argsMatrix.items():
    newCommands = []
    for command in scenarioCommands:
        for v in val:
            newCommands.append(f'{command} -{arg}={v}')
    scenarioCommands = newCommands

print("Commands to run :\n"+ "\n".join(scenarioCommands))

def run_command(command):
    print(f"Spawning {command}")
    # Change directory to '~/ns3' and execute the command using waf
    process = subprocess.Popen(
        f'./waf --run "{command}"',
        shell=True,
        cwd=os.path.expanduser('~/ns3')
    )
    process.wait()

if __name__ == '__main__':
    # Create a process pool with a maximum of X processes
    with ProcessPoolExecutor(max_workers=6) as executor:
        # Submit all commands to the pool
        futures = [executor.submit(run_command, cmd) for cmd in scenarioCommands]
        # Use tqdm to display a progress bar as the commands complete
        for future in tqdm(as_completed(futures), total=len(futures)):
            try:
                future.result()
            except Exception as e:
                print(f"Command failed with error: {e}")
