from pathlib import Path
import os
import time
from typing import Optional

def select_scenario(scenario_name:Optional[str] = None, latest:bool=False) -> Path:
    # Get all folder names from the logs folder
    logs_folder = Path(os.path.abspath('')) / 'logs'
    scenarios = sorted([folder for folder in logs_folder.iterdir() if folder.is_dir()])
    if scenario_name is None:
        print('Select a scenario:')
        for i, folder in enumerate(scenarios):
            print(f"[{i}]: {folder.name}")
        scenario_selection = int(input("Input which scenario you want to visualize: "))
    else:
        scenario_selection = None
        for i, folder in enumerate(scenarios):
            if folder.name == scenario_name:
                scenario_selection = i
                break
        if scenario_selection is None:
            raise ValueError(f"Scenario {scenario_name} not found")
    runs_folder = scenarios[scenario_selection]
    print(f"Selected scenario: {runs_folder.name}")
    print()
    runs = sorted([folder for folder in runs_folder.iterdir() if folder.is_dir()])
    if not latest:
        time.sleep(1)
        print("Select a test run:")
        for i, folder in enumerate(runs):
            print(f"[{i}]: {folder.name}")
        run_selection = int(input("Input which run you want to visualise: "))
    else:
        run_selection = len(runs) - 1
    print(f"Selected run: {runs[run_selection].name}")
    return runs[run_selection]