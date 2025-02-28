from pathlib import Path
import os
import time

def select_scenario() -> Path:
    # Get all folder names from the logs folder
    logs_folder = Path(os.path.abspath('')) / 'logs'
    scenarios = sorted([folder for folder in logs_folder.iterdir() if folder.is_dir()])
    print('Select a scenario:')
    for i, folder in enumerate(scenarios):
        print(f"[{i}]: {folder.name}")
    scenario_selection = int(input("Input which scenario you want to visualize: "))
    runs_folder = scenarios[scenario_selection]
    print(f"Selected scenario: {runs_folder.name}")
    print()
    time.sleep(1)
    print("Select a test run:")
    runs = sorted([folder for folder in runs_folder.iterdir() if folder.is_dir()])
    for i, folder in enumerate(runs):
        print(f"[{i}]: {folder.name}")
    run_selection = int(input("Input which run you want to visualise: "))
    print(f"Selected run: {runs[run_selection].name}")
    return runs[run_selection]