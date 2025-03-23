#!/usr/bin/env python3
import curses
import subprocess
import os
import time
import select
import numpy as np

def main(stdscr):
    # Setup curses
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.clear()
    height, width = stdscr.getmaxyx()

    # Prepare the list of commands
    scenario_name = "unified"
    args_matrix = {
        "nodeXCount" : range(1, 5),
        "nodeXSpacing" : list(np.arange(0.5, 2.5, 0.1)) + list(np.arange(5, 25, 10)),
        "nodeYCount" : range(1, 5),
        "nodeYSpacing" : list(np.arange(0.5, 2.5, 0.1)) + list(np.arange(5, 25, 10)),
        "nodeZCount" : [1],
        "nodeZSpacing" : [1],
        "uavSpeed" : list(np.arange(0.0, 1.2, 0.05)) + list(np.arange(1.3, 2.5, 0.1)),
        "propagationModel" : ["freshwater"],
        "stopTime" : [30*60], # seconds
        "channelWidth" : ["1 --dataRatePHY=OfdmRate1_2MbpsBW1MHz", "2 --dataRatePHY=OfdmRate7_8MbpsBW2MHz"],
    }

    scenario_commands = [f'./waf --run "{scenario_name}']
    for arg, val in args_matrix.items():
        new_commands = []
        for command in scenario_commands:
            for v in val:
                new_commands.append(f'{command} --{arg}={v}')
        scenario_commands = new_commands
        
    scenario_commands = [f'{command}"' for command in scenario_commands]
    total_commands = len(scenario_commands)

    # --- GRID LAYOUT SETUP ---
    # Arrange six worker windows in 2 rows x 3 columns.
    grid_rows = 2
    grid_cols = 3
    max_workers = grid_rows * grid_cols

    # Reserve one line at the bottom for the progress bar.
    progress_win_height = 1

    # The grid occupies all rows except the progress bar.
    grid_total_height = height - progress_win_height
    grid_total_width = width

    # Define gaps between windows.
    gap_y = 1  # gap (in rows) between grid rows
    gap_x = 1  # gap (in columns) between grid columns

    # Compute each worker window’s dimensions.
    worker_win_height = (grid_total_height - (grid_rows - 1) * gap_y) // grid_rows
    worker_win_width = (grid_total_width - (grid_cols - 1) * gap_x) // grid_cols

    # Create worker windows arranged in a grid.
    worker_windows = []
    for r in range(grid_rows):
        for c in range(grid_cols):
            start_y = r * (worker_win_height + gap_y)
            start_x = c * (worker_win_width + gap_x)
            win = curses.newwin(worker_win_height, worker_win_width, start_y, start_x)
            win.scrollok(True)
            worker_windows.append(win)

    # Draw separating lines on the main screen (stdscr) in the gap areas.
    # Horizontal separators
    for r in range(1, grid_rows):
        sep_y = r * (worker_win_height + gap_y) - gap_y
        stdscr.hline(sep_y, 0, curses.ACS_HLINE, grid_total_width)
    # Vertical separators
    for c in range(1, grid_cols):
        sep_x = c * (worker_win_width + gap_x) - gap_x
        stdscr.vline(0, sep_x, curses.ACS_VLINE, grid_total_height)
    stdscr.refresh()

    # Create the progress bar window at the bottom.
    progress_win = curses.newwin(progress_win_height, width, height - progress_win_height, 0)

    # --- PROCESS MANAGEMENT ---
    # Each worker slot will hold a dict with:
    #    'proc'    : subprocess.Popen object
    #    'command' : the command being executed
    #    'window'  : the assigned curses window
    workers = [None] * max_workers
    command_index = 0  # next command to run
    finished_commands = 0

    def start_command(i):
        nonlocal command_index
        if command_index < total_commands:
            cmd = scenario_commands[command_index]
            # Clear the window and display the command that is starting.
            worker_windows[i].clear()
            try:
                worker_windows[i].addstr(0, 0, f"Spawning: {cmd}\n")
            except curses.error:
                pass
            worker_windows[i].refresh()
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
            workers[i] = {'proc': proc, 'command': cmd, 'window': worker_windows[i]}
            command_index += 1

    # Start initial processes up to max_workers.
    for i in range(max_workers):
        start_command(i)

    # Main loop: update windows with subprocess output and update the progress bar.
    while finished_commands < total_commands:
        for i, worker in enumerate(workers):
            if worker is None:
                continue
            proc = worker['proc']
            win = worker['window']
            # Non-blocking read of process output.
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
            # If the process has finished, grab any remaining output and start a new command.
            if proc.poll() is not None:
                remaining = proc.stdout.read()
                if remaining:
                    remaining = remaining.replace("\0", "")
                    try:
                        win.addstr(remaining)
                    except curses.error:
                        pass
                    win.refresh()
                finished_commands += 1
                workers[i] = None  # free the worker slot
                start_command(i)  # start next command if available

        # Update the progress bar at the bottom.
        progress = int((finished_commands / total_commands) * (width - 20))
        progress_bar = "[" + "#" * progress + " " * (width - 20 - progress) + "]"
        progress_win.clear()
        try:
            progress_win.addstr(0, 0, f"Progress: {progress_bar} {finished_commands}/{total_commands}")
        except curses.error:
            pass
        progress_win.refresh()
        
        # Short sleep and allow for a key press to quit early (press 'q' to exit).
        time.sleep(0.05)
        try:
            ch = stdscr.getch()
            if ch == ord('q'):
                break
        except curses.error:
            pass

    # Ensure all subprocesses have terminated.
    for worker in workers:
        if worker and worker['proc']:
            worker['proc'].wait()

    stdscr.nodelay(False)
    stdscr.addstr(height - 1, 0, "All processes complete. Press any key to exit.")
    stdscr.refresh()
    stdscr.getch()

if __name__ == '__main__':
    curses.wrapper(main)
