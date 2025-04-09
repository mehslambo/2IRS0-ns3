#!/usr/bin/env python3
import curses
import curses.textpad
import subprocess
import os
import time
import select
import numpy as np
import re

# Copy the current environment variables.
env = os.environ.copy()
env["LD_LIBRARY_PATH"] = os.path.expanduser('~/ns3/build') + ":" + env.get("LD_LIBRARY_PATH", "")

def get_scenario_commands():
    scenario_name = "unified"
    
    # Independent variables.
    independent_vars = {
        "nodeXCount" : list(np.arange(1, 26, 1)),
        "nodeYCount" : list(np.arange(1, 26, 1)),
        "nodeZCount": [1],
        "nodeZSpacing": [1],
        "nodeZOffset": list(np.arange(0, -1.8, -0.1)),
        "auvSpeed": [0],
        "propagationModel": ["underwater"],
        "waterTemperature": [20],
        "waterSalinity": [0.01],
        "stopTime": [10*60],  # seconds
        "channelWidth": [1],
        "scenarioFolderPath": ["\"unified/buoy/run\""]
    }

    # Dependent variables defined via lambda functions.
    dependent_vars = {
        "nodeXSpacing": lambda args: 1.8 / (args["nodeXCount"] - 1) if args["nodeXCount"] > 1 else 1,
        "nodeXOffset": lambda args: -args["nodeXSpacing"] * (args["nodeXCount"] - 1) / 2,
        "nodeYSpacing": lambda args: 1.8 / (args["nodeYCount"] - 1) if args["nodeYCount"] > 1 else 1,
        "nodeYOffset": lambda args: -args["nodeYSpacing"] * (args["nodeYCount"] - 1) / 2,
        "dataRatePHY": lambda args: "OfdmRate1_2MbpsBW1MHz" if args["channelWidth"] == 1 else "OfdmRate7_8MbpsBW2MHz",
    }

    # Build all combinations of independent variables.
    configurations = [{}]
    for key, values in independent_vars.items():
        new_configurations = []
        for config in configurations:
            for value in values:
                new_config = config.copy()
                new_config[key] = value
                new_configurations.append(new_config)
        configurations = new_configurations

    for config in configurations:
        for dep_key, formula in dependent_vars.items():
            config[dep_key] = formula(config)

    configurations = [config for config in configurations if config["nodeYCount"] <= config["nodeXCount"]]

    scenario_commands = []
    for config in configurations:
        command = f'./build/scratch/unified/unified {scenario_name}'
        for arg, value in config.items():
            command += f' --{arg}={value}'
        scenario_commands.append(command)

    return scenario_commands

def format_time(seconds):
    """Format seconds as HH:MM:SS."""
    seconds = int(round(seconds))
    h, remainder = divmod(seconds, 3600)
    m, s = divmod(remainder, 60)
    return f"{h:02d}:{m:02d}:{s:02d}"

def parse_progress_line(line):
    """
    Parse a progress line of the form:
    Sim time: 585.0/600.0s (95.4%) [=============>     ] Est: 1h 1m 1s
    Returns a tuple (full_progress_text, est_remaining_in_seconds).
    """
    match = re.search(r'Est:\s*((?P<hours>\d+)h)?\s*((?P<minutes>\d+)m)?\s*((?P<seconds>\d+)s)?', line)
    if match:
        hours = int(match.group('hours')) if match.group('hours') else 0
        minutes = int(match.group('minutes')) if match.group('minutes') else 0
        seconds = int(match.group('seconds')) if match.group('seconds') else 0
        total_est = hours * 3600 + minutes * 60 + seconds
        return line.strip(), total_est
    else:
        return line.strip(), None

def main(stdscr):
    enable_logging = True

    # Initialize curses colors.
    curses.start_color()
    curses.use_default_colors()
    curses.init_pair(1, curses.COLOR_WHITE, -1)   # queued
    curses.init_pair(2, curses.COLOR_YELLOW, -1)    # running
    curses.init_pair(3, curses.COLOR_GREEN, -1)     # success
    curses.init_pair(4, curses.COLOR_RED, -1)       # failed
    curses.init_pair(5, curses.COLOR_BLACK, curses.COLOR_WHITE)  # selected/highlight

    # Enable mouse events.
    curses.mousemask(curses.ALL_MOUSE_EVENTS | curses.REPORT_MOUSE_POSITION)
    curses.curs_set(0)
    stdscr.nodelay(True)
    stdscr.clear()

    if enable_logging:
        log_dir = os.path.join("postprocessing", "stdout")
        os.makedirs(log_dir, exist_ok=True)
    else:
        log_dir = None

    # Dynamic layout: side panel is 1/3 of terminal, main panel is 2/3.
    # Increase the progress bar area to 2 lines to include help text.
    progress_bar_height = 2
    height, width = stdscr.getmaxyx()
    side_panel_width = width // 3
    main_panel_height = height - progress_bar_height
    main_panel_width = width - side_panel_width

    side_panel_win = curses.newwin(main_panel_height, side_panel_width, 0, 0)
    main_panel_win = curses.newwin(main_panel_height, main_panel_width, 0, side_panel_width)
    progress_win = curses.newwin(progress_bar_height, width, height - progress_bar_height, 0)

    # Build the list of commands.
    scenario_commands = get_scenario_commands()
    total_commands = len(scenario_commands)

    # Each command's state stores its original command, progress info, output, etc.
    commands_state = []
    for cmd in scenario_commands:
        commands_state.append({
            'command': cmd,
            'status': 'queued',
            'output': [],
            'progress_line': None,
            'est_remaining': None,
            'proc': None,
            'log_file': None,
            'exit_code': None
        })

    # Concurrency settings.
    max_workers = 6  # Initial number of workers.
    next_command_index = 0
    finished_commands = 0

    current_selected_index = 0
    side_scroll_offset = 0

    start_time = time.time()
    default_est = 60  # Fallback estimated remaining time.
    
    # Toggle flag for filtering only in-progress commands.
    filter_in_progress = False

    def start_command(index):
        """
        Starts the command at the given index.
        """
        state = commands_state[index]
        cmd = state['command']
        state['status'] = 'running'
        spawn_msg = f"Spawning: {cmd}\n"
        state['output'].append(spawn_msg)
        if enable_logging and log_dir is not None:
            log_filename = os.path.join(log_dir, f"process_{index}.txt")
            try:
                log_file = open(log_filename, "w")
            except Exception:
                log_file = None
            state['log_file'] = log_file
            if log_file:
                log_file.write(spawn_msg)
                log_file.flush()
        proc = subprocess.Popen(
            cmd,
            shell=True,
            cwd=os.path.expanduser('~/ns3'),
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1,
            env=env
        )
        state['proc'] = proc

    def start_available_commands():
        """
        Launch queued commands until the current running count is below max_workers.
        """
        nonlocal next_command_index
        running = sum(1 for s in commands_state if s['status'] == 'running')
        while running < max_workers and next_command_index < total_commands:
            start_command(next_command_index)
            next_command_index += 1
            running = sum(1 for s in commands_state if s['status'] == 'running')

    start_available_commands()

    def compute_overall_eta():
        """
        Compute overall ETA by simulating the assignment of tasks across worker slots.
        Uses the parsed estimated remaining times for running tasks and defaults otherwise.
        """
        running_ests = [s['est_remaining'] for s in commands_state if s['status'] == 'running' and s['est_remaining'] is not None]
        avg_est = sum(running_ests) / len(running_ests) if running_ests else default_est
        tasks = []
        for s in commands_state:
            if s['status'] in ('success', 'failed'):
                remaining = 0
            elif s['status'] == 'running':
                remaining = s['est_remaining'] if s['est_remaining'] is not None else default_est
            elif s['status'] == 'queued':
                remaining = avg_est
            tasks.append(remaining)
        slots = [0] * max_workers
        for t in tasks:
            min_slot = min(slots)
            slot_index = slots.index(min_slot)
            slots[slot_index] += t
        overall_remaining = max(slots)
        return overall_remaining

    def draw_side_panel():
        """
        Draw the side panel with command number and progress, plus a scrollbar.
        If filtering is active, only in-progress (running) commands are shown.
        """
        nonlocal side_scroll_offset, current_selected_index
        
        side_panel_win.erase()
        # Determine the list of indices to display.
        if filter_in_progress:
            filtered = [i for i, s in enumerate(commands_state) if s['status'] == 'running']
            # If current selection is not in the filtered list, select the first available.
            if filtered:
                if current_selected_index not in filtered:
                    current_selected_index = filtered[0]
            else:
                filtered = []
        else:
            filtered = list(range(total_commands))
        
        total_display = len(filtered)
        visible_lines = main_panel_height - 2  # Interior without border.
        if filtered:
            try:
                current_filtered_index = filtered.index(current_selected_index)
            except ValueError:
                current_filtered_index = 0
                current_selected_index = filtered[0]
        else:
            current_filtered_index = 0

        # Adjust scroll offset based on filtered list.
        if current_filtered_index < side_scroll_offset:
            side_scroll_offset = current_filtered_index
        elif current_filtered_index >= side_scroll_offset + visible_lines:
            side_scroll_offset = current_filtered_index - visible_lines + 1

        # Draw each entry from the filtered list.
        for disp_index, full_index in enumerate(filtered):
            if disp_index < side_scroll_offset or disp_index >= side_scroll_offset + visible_lines:
                continue
            display_line = disp_index - side_scroll_offset
            state = commands_state[full_index]
            if state.get('progress_line'):
                display_text = state['progress_line'].replace("Sim time:", "").strip()
                display_text = re.sub(r'\s*\[.*?\]', '', display_text).strip()
            else:
                display_text = state['command']
            max_text_width = side_panel_width - 9  # Leave room for a 6-digit number and a space.
            if len(display_text) > max_text_width:
                display_text = display_text[:max_text_width - 3] + "..."
            if state['status'] == 'queued':
                color = curses.color_pair(1)
            elif state['status'] == 'running':
                color = curses.color_pair(2)
            elif state['status'] == 'success':
                color = curses.color_pair(3)
            elif state['status'] == 'failed':
                color = curses.color_pair(4)
            else:
                color = curses.A_NORMAL
            if full_index == current_selected_index:
                color |= curses.A_REVERSE
            try:
                side_panel_win.addstr(display_line + 1, 1, f"{full_index:06d} {display_text}", color)
            except curses.error:
                pass

        # Draw vertical scrollbar if needed.
        if total_display > visible_lines:
            track_height = visible_lines
            thumb_size = max(1, int(track_height * (visible_lines / total_display)))
            effective_scroll_range = total_display - visible_lines
            thumb_position = int((side_scroll_offset / max(1, effective_scroll_range)) * (track_height - thumb_size))
            for j in range(track_height):
                if thumb_position <= j < thumb_position + thumb_size:
                    try:
                        side_panel_win.addch(j + 1, side_panel_width - 2, ' ', curses.A_REVERSE)
                    except curses.error:
                        pass
                else:
                    try:
                        side_panel_win.addch(j + 1, side_panel_width - 2, '|')
                    except curses.error:
                        pass

        side_panel_win.box()
        side_panel_win.refresh()

    def draw_main_panel():
        """
        Draw the main panel showing output for the selected command.
        """
        main_panel_win.erase()
        state = commands_state[current_selected_index]
        output_lines = state['output']
        max_lines = main_panel_height - 2
        for i, line in enumerate(output_lines[-max_lines:]):
            try:
                main_panel_win.addstr(i+1, 1, line.strip())
            except curses.error:
                pass
        main_panel_win.box()
        title = f" Output for command {current_selected_index} "
        try:
            main_panel_win.addstr(0, 2, title)
        except curses.error:
            pass
        main_panel_win.refresh()

    def draw_progress_bar():
        """
        Draw the progress bar with overall progress, ETA, worker count, and help text.
        """
        overall_eta = compute_overall_eta()
        eta_str = f"ETA: {format_time(overall_eta)}"
        bar_width = main_panel_width  # Use main panel width.
        progress = int((finished_commands / total_commands) * bar_width)
        progress_bar = "[" + "#" * progress + " " * (bar_width - progress) + "]"
        progress_win.erase()
        try:
            progress_win.addstr(0, 0,
                f"Progress: {progress_bar} {finished_commands}/{total_commands} {eta_str} | Workers: {max_workers}")
        except curses.error:
            pass
        help_text = (f"Keys: q=Quit | Up/Down=Select | p=Add worker | m=Remove worker | "
                     f"f=Toggle filter ({'On' if filter_in_progress else 'Off'}) | Mouse=Select")
        try:
            progress_win.addstr(1, 0, help_text[:width-1])
        except curses.error:
            pass
        progress_win.refresh()

    # Main loop.
    while finished_commands < total_commands or any(s['status'] == 'running' for s in commands_state):
        # Auto-update dimensions if the terminal has been resized.
        new_height, new_width = stdscr.getmaxyx()
        if new_height != height or new_width != width:
            height, width = new_height, new_width
            side_panel_width = width // 3
            main_panel_height = height - progress_bar_height
            main_panel_width = width - side_panel_width
            side_panel_win.resize(main_panel_height, side_panel_width)
            main_panel_win.resize(main_panel_height, main_panel_width)
            main_panel_win.mvwin(0, side_panel_width)
            progress_win.resize(progress_bar_height, width)
            progress_win.mvwin(height - progress_bar_height, 0)
            stdscr.erase()
            stdscr.refresh()

        try:
            ch = stdscr.getch()
        except Exception:
            ch = -1

        # Handle keybinds.
        if ch != -1:
            if ch == ord('q'):
                break
            elif ch == curses.KEY_UP:
                if current_selected_index > 0:
                    current_selected_index -= 1
            elif ch == curses.KEY_DOWN:
                if current_selected_index < total_commands - 1:
                    current_selected_index += 1
            elif ch == ord('p'):
                max_workers += 1
            elif ch == ord('m'):
                if max_workers > 1:
                    max_workers -= 1
            elif ch == ord('f'):
                filter_in_progress = not filter_in_progress
                if filter_in_progress:
                    running_indices = [i for i, s in enumerate(commands_state) if s['status'] == 'running']
                    if running_indices:
                        if current_selected_index not in running_indices:
                            current_selected_index = running_indices[0]
            elif ch == curses.KEY_MOUSE:
                try:
                    _, mx, my, _, _ = curses.getmouse()
                    if mx < side_panel_width and my < main_panel_height:
                        if filter_in_progress:
                            filtered = [i for i, s in enumerate(commands_state) if s['status'] == 'running']
                            if filtered:
                                clicked_index = my - 1
                                if clicked_index < len(filtered):
                                    current_selected_index = filtered[clicked_index]
                        else:
                            clicked_index = my - 1 + side_scroll_offset
                            if 0 <= clicked_index < total_commands:
                                current_selected_index = clicked_index
                except Exception:
                    pass

        start_available_commands()

        for idx, state in enumerate(commands_state):
            if state['status'] != 'running' or state['proc'] is None:
                continue
            proc = state['proc']
            if proc.stdout:
                try:
                    rlist, _, _ = select.select([proc.stdout], [], [], 0.05)
                except Exception:
                    rlist = []
                if rlist:
                    line = proc.stdout.readline().replace("\0", "")
                    if line:
                        state['output'].append(line)
                        if line.strip().startswith("Sim time:"):
                            progress, est = parse_progress_line(line)
                            state['progress_line'] = progress
                            if est is not None:
                                state['est_remaining'] = est
                        if enable_logging and state['log_file']:
                            try:
                                state['log_file'].write(line)
                                state['log_file'].flush()
                            except Exception:
                                pass
            if proc.poll() is not None:
                remaining = proc.stdout.read() if proc.stdout else ""
                if remaining:
                    remaining = remaining.replace("\0", "")
                    for l in remaining.splitlines():
                        state['output'].append(l)
                        if l.strip().startswith("Sim time:"):
                            progress, est = parse_progress_line(l)
                            state['progress_line'] = progress
                            if est is not None:
                                state['est_remaining'] = est
                        if enable_logging and state['log_file']:
                            try:
                                state['log_file'].write(l + "\n")
                                state['log_file'].flush()
                            except Exception:
                                pass
                state['exit_code'] = proc.returncode
                state['status'] = 'success' if proc.returncode == 0 else 'failed'
                finished_commands += 1
                if enable_logging and state['log_file']:
                    try:
                        state['log_file'].close()
                    except Exception:
                        pass
                state['proc'] = None

        draw_side_panel()
        draw_main_panel()
        draw_progress_bar()
        time.sleep(0.05)

    for state in commands_state:
        if state['proc']:
            state['proc'].wait()
            if enable_logging and state['log_file'] and not state['log_file'].closed:
                state['log_file'].close()

    stdscr.nodelay(False)
    stdscr.erase()
    final_msg = "All processes complete. Press any key to exit."
    stdscr.addstr(height // 2, (width - len(final_msg)) // 2, final_msg)
    stdscr.refresh()
    stdscr.getch()

if __name__ == '__main__':
    curses.wrapper(main)
