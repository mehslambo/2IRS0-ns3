#!/usr/bin/env python3
import curses
import subprocess
import os
import time
import select
import numpy as np

def get_scenario_commands():
    scenario_name = "unified"
    
    # Independent variables
    independent_vars = {
        "nodeXCount" : [1],
        "nodeXSpacing": [1],
        "nodeXOffset": list(np.arange(0, 1.5, 0.05)) + list(np.arange(1.5, 1.9, 0.01)) + list(np.arange(1.9, 2.3, 0.05)),
        "nodeYCount" : [1],
        "nodeYSpacing": [1],
        "nodeYOffset": [0],
        "nodeZCount": [1],
        "nodeZSpacing": [1],
        "nodeZOffset": [0],
        "auvSpeed": [0],
        "propagationModel": ["underwater"],
        "waterTemperature": list(np.arange(0, 30, .5)),
        "waterSalinity": [0.01, 0.5],
        "stopTime": [10*60],  # seconds
        "channelWidth": [1, 2],
        "packetStatsConfig": ["means"],
        "enablePositionLogging": ["true"],
        "powerLoggingConfig": ["short"],
        "enableMacStats": ["true"],
        "scenarioFolderPath": ["unified/p2p/run"]
    }

    # Dependent variables defined via lambda functions. 
    # Each lambda receives a dict with the current independent variables.
    dependent_vars = {
        "dataRatePHY": lambda args: "OfdmRate1_2MbpsBW1MHz" if args["channelWidth"] == 1 else "OfdmRate7_8MbpsBW2MHz",
    }

    # Build all combinations of independent variables as a list of configuration dictionaries.
    configurations = [{}]
    for key, values in independent_vars.items():
        new_configurations = []
        for config in configurations:
            for value in values:
                new_config = config.copy()
                new_config[key] = value
                new_configurations.append(new_config)
        configurations = new_configurations

    # For each configuration, calculate dependent variables.
    for config in configurations:
        for dep_key, formula in dependent_vars.items():
            config[dep_key] = formula(config)

    # Build the scenario commands by appending flags for each configuration.
    scenario_commands = []
    for config in configurations:
        command = f'./waf --run "{scenario_name}'
        for arg, value in config.items():
            command += f' --{arg}={value}'
        command += '"'
        scenario_commands.append(command)

    return scenario_commands


def format_time(seconds):
    # Format seconds as HH:MM:SS.
    seconds = int(seconds)
    h, remainder = divmod(seconds, 3600)
    m, s = divmod(remainder, 60)
    return f"{h:02d}:{m:02d}:{s:02d}"

def main(stdscr):
    enable_logging = True

    # Setup curses
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.clear()
    height, width = stdscr.getmaxyx()

    # Create directory for log files if logging is enabled.
    if enable_logging:
        log_dir = os.path.join("postprocessing", "stdout")
        os.makedirs(log_dir, exist_ok=True)
    else:
        log_dir = None

    # Prepare the list of commands
    scenario_commands = get_scenario_commands()
    total_commands = len(scenario_commands)

    # --- GRID LAYOUT SETUP ---
    grid_rows = 2
    grid_cols = 3
    max_workers = grid_rows * grid_cols

    progress_win_height = 1
    grid_total_height = height - progress_win_height
    grid_total_width = width

    gap_y = 1  # vertical gap
    gap_x = 1  # horizontal gap

    worker_win_height = (grid_total_height - (grid_rows - 1) * gap_y) // grid_rows
    worker_win_width = (grid_total_width - (grid_cols - 1) * gap_x) // grid_cols

    worker_windows = []
    for r in range(grid_rows):
        for c in range(grid_cols):
            start_y = r * (worker_win_height + gap_y)
            start_x = c * (worker_win_width + gap_x)
            win = curses.newwin(worker_win_height, worker_win_width, start_y, start_x)
            win.scrollok(True)
            worker_windows.append(win)

    # Draw grid separators
    for r in range(1, grid_rows):
        sep_y = r * (worker_win_height + gap_y) - gap_y
        stdscr.hline(sep_y, 0, curses.ACS_HLINE, grid_total_width)
    for c in range(1, grid_cols):
        sep_x = c * (worker_win_width + gap_x) - gap_x
        stdscr.vline(0, sep_x, curses.ACS_VLINE, grid_total_height)
    stdscr.refresh()

    progress_win = curses.newwin(progress_win_height, width, height - progress_win_height, 0)

    # --- PROCESS MANAGEMENT ---
    # Each worker slot holds:
    #   'proc'    : subprocess.Popen object
    #   'command' : the command being executed
    #   'window'  : the assigned curses window
    #   'log_file': file handle for the log output (if logging is enabled)
    workers = [None] * max_workers
    command_index = 0  # next command to run
    finished_commands = 0

    def start_command(i):
        nonlocal command_index
        if command_index < total_commands:
            cmd = scenario_commands[command_index]
            # Clear window and display the starting command.
            worker_windows[i].clear()
            try:
                worker_windows[i].addstr(0, 0, f"Spawning: {cmd}\n")
            except curses.error:
                pass
            worker_windows[i].refresh()
            # Open a log file for this command if logging is enabled.
            if enable_logging and log_dir is not None:
                log_filename = os.path.join(log_dir, f"process_{command_index}.txt")
                log_file = open(log_filename, "w")
                log_file.write(f"Spawning: {cmd}\n")
            else:
                log_file = None
            # Start the process; stdout and stderr are merged.
            proc = subprocess.Popen(
                cmd,
                shell=True,
                cwd=os.path.expanduser('~/ns3'),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                universal_newlines=True,
                bufsize=1
            )
            workers[i] = {
                'proc': proc,
                'command': cmd,
                'window': worker_windows[i],
                'log_file': log_file
            }
            command_index += 1

    # Start initial processes up to max_workers.
    for i in range(max_workers):
        start_command(i)

    # Record start time for ETA calculation.
    start_time = time.time()

    # Main loop: update windows and progress bar.
    while finished_commands < total_commands:
        for i, worker in enumerate(workers):
            if worker is None:
                continue
            proc = worker['proc']
            win = worker['window']
            log_file = worker['log_file']
            # Non-blocking read from process output.
            if proc.stdout:
                ready, _, _ = select.select([proc.stdout], [], [], 0.05)
                if ready:
                    line = proc.stdout.readline().replace("\0", "")
                    if line:
                        try:
                            win.addstr(line)
                        except curses.error:
                            pass
                        win.refresh()
                        if enable_logging and log_file is not None:
                            log_file.write(line)
                            log_file.flush()
            # Check if process has finished.
            if proc.poll() is not None:
                # Read any remaining output.
                remaining = proc.stdout.read()
                if remaining:
                    remaining = remaining.replace("\0", "")
                    try:
                        win.addstr(remaining)
                    except curses.error:
                        pass
                    win.refresh()
                    if enable_logging and log_file is not None:
                        log_file.write(remaining)
                        log_file.flush()
                finished_commands += 1
                if enable_logging and log_file is not None:
                    log_file.close()
                workers[i] = None  # free the slot
                start_command(i)  # start next command if available

        # Update the progress bar.
        elapsed = time.time() - start_time
        if finished_commands == 0:
            eta_str = "ETA: Calculating..."
        else:
            estimated_total = elapsed * total_commands / finished_commands
            eta = estimated_total - elapsed
            eta_str = f"ETA: {format_time(eta)}"
        progress = int((finished_commands / total_commands) * (width - 40))
        progress_bar = "[" + "#" * progress + " " * (width - 40 - progress) + "]"
        progress_win.clear()
        try:
            progress_win.addstr(0, 0, f"Progress: {progress_bar} {finished_commands}/{total_commands} {eta_str}")
        except curses.error:
            pass
        progress_win.refresh()
        
        time.sleep(0.05)
        try:
            ch = stdscr.getch()
            if ch == ord('q'):
                break
        except curses.error:
            pass

    # Ensure all subprocesses have terminated and close any open log files.
    for worker in workers:
        if worker and worker['proc']:
            worker['proc'].wait()
            if enable_logging and worker['log_file'] is not None and not worker['log_file'].closed:
                worker['log_file'].close()

    stdscr.nodelay(False)
    stdscr.addstr(height - 1, 0, "All processes complete. Press any key to exit.")
    stdscr.refresh()
    stdscr.getch()

if __name__ == '__main__':
    curses.wrapper(main)
