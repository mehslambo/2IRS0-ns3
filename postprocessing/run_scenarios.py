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
    
    # Independent variables
    independent_vars = {
        "nodeXCount" : [1],
        "nodeXSpacing": [1],
        "nodeXOffset": list(np.arange(0, 2.3, 0.015)),
        "nodeYCount" : [1],
        "nodeYSpacing": [1],
        "nodeYOffset": [0],
        "nodeZCount": [1],
        "nodeZSpacing": [1],
        "nodeZOffset": [0],
        "auvSpeed": [0],
        "propagationModel": ["underwater"],
        "waterTemperature": list(np.arange(0, 30)),
        "waterSalinity": [0.01, 0.5],
        "stopTime": [10*60],  # seconds
        "channelWidth": [1, 2],
        "scenarioFolderPath": ["\"unified/p2p/run\""]
    }

    # Dependent variables defined via lambda functions. 
    # Each lambda receives a dict with the current independent variables.
    dependent_vars = {
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
            'exit_code': None,
            'start_time': None,
            'duration': None,
            'finish_time': None,
        })

    # Concurrency settings.
    max_workers = 6  # Initial number of workers.
    next_command_index = 0
    finished_commands = 0

    current_selected_index = 0
    side_scroll_offset = 0

    start_time = time.time()
    default_est = 60  # Fallback estimated remaining time.

    # Replace the filter boolean with a filter mode.
    # filter_options: None stands for ALL; otherwise filter by 'queued', 'running', 'success', or 'failed'.
    filter_options = [None, "queued", "running", "success", "failed"]
    filter_mode_index = 0
    current_filter = filter_options[filter_mode_index]

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
        state['start_time'] = time.time()
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
        Compute overall ETA based on:
          - True execution durations for finished tasks,
          - Current estimates (or computed remaining time) for running tasks,
          - Estimated time for queued tasks.
        Then simulate scheduling these tasks among worker slots.
        """
        current_time = time.time()
        # Gather true execution times of finished commands.
        finished_durations = [s['duration'] for s in commands_state 
                              if s['status'] in ('success', 'failed') and s['duration'] is not None]
        if finished_durations:
            avg_finished = sum(finished_durations) / len(finished_durations)
        else:
            avg_finished = default_est

        tasks = []
        for s in commands_state:
            if s['status'] in ('success', 'failed'):
                rem = 0
            elif s['status'] == 'running':
                if s.get('est_remaining') is not None:
                    rem = s['est_remaining']
                elif s.get('start_time') is not None:
                    # Estimate remaining by subtracting elapsed time from average finished duration.
                    elapsed = current_time - s['start_time']
                    rem = max(avg_finished - elapsed, 0)
                else:
                    rem = avg_finished
            elif s['status'] == 'queued':
                rem = avg_finished
            tasks.append(rem)
        # Simulate assigning tasks to worker slots.
        slots = [0] * max_workers
        for t in tasks:
            # Assign task to the worker with the minimum total time.
            min_slot = min(slots)
            slot_index = slots.index(min_slot)
            slots[slot_index] += t
        overall_remaining = max(slots)
        return overall_remaining

    def draw_side_panel():
        """
        Draw the side panel with command number and progress, plus a scrollbar.
        The displayed commands are filtered based on the current_filter:
          - If current_filter is None, show all commands;
          - Otherwise, show only commands whose 'status' matches current_filter.
        """
        nonlocal side_scroll_offset, current_selected_index
        
        side_panel_win.erase()
        # Determine the list of indices to display based on filter.
        if current_filter is None:
            filtered = list(range(total_commands))
        else:
            filtered = [i for i, s in enumerate(commands_state) if s['status'] == current_filter]
            if filtered:
                if current_selected_index not in filtered:
                    current_selected_index = filtered[0]

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

        # Adjust scroll offset based on the filtered list.
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
        filter_label = "ALL" if current_filter is None else current_filter.upper()
        help_text = (f"Keys: q=Quit | Up/Down=Select | a=Add worker | r=Remove worker | "
                     f"f=Toggle filter ({filter_label}) | Mouse=Select")
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
            # Switch add and remove worker keys to 'a' and 'r'
            elif ch == ord('a'):
                max_workers += 1
            elif ch == ord('r'):
                if max_workers > 1:
                    max_workers -= 1
            elif ch == ord('f'):
                # Cycle through filter options: None (ALL), queued, running, success, failed.
                filter_mode_index = (filter_mode_index + 1) % len(filter_options)
                current_filter = filter_options[filter_mode_index]
                if current_filter is not None:
                    filtered = [i for i, s in enumerate(commands_state) if s['status'] == current_filter]
                    if filtered and current_selected_index not in filtered:
                        current_selected_index = filtered[0]
            elif ch == curses.KEY_MOUSE:
                try:
                    _, mx, my, _, _ = curses.getmouse()
                    if mx < side_panel_width and my < main_panel_height:
                        if current_filter is None:
                            clicked_index = my - 1 + side_scroll_offset
                            if 0 <= clicked_index < total_commands:
                                current_selected_index = clicked_index
                        else:
                            filtered = [i for i, s in enumerate(commands_state) if s['status'] == current_filter]
                            if filtered:
                                clicked_index = my - 1
                                if clicked_index < len(filtered):
                                    current_selected_index = filtered[clicked_index]
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
                state['finish_time'] = time.time()
                if state['start_time'] is not None:
                    state['duration'] = state['finish_time'] - state['start_time']
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
