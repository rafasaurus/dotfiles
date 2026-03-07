#!/usr/bin/env python3
import os
from datetime import date

directory = os.path.expanduser("~/obsidian/weekly-todos")
os.makedirs(directory, exist_ok=True)

# Sweeps past weekly notes for unfinished tasks and rolls them into the current week.

year, week, _ = date.today().isocalendar()
current_file = os.path.join(directory, f"{year}-W{week:02d}.md")

if os.path.exists(current_file):
    with open(current_file, "r") as f:
        current_content = f.read()
else:
    current_content = f"# {year}-W{week:02d}\n\n"

pending_tasks = []
origin_weeks = set()
updates = {}

for filename in sorted(os.listdir(directory)):
    if not filename.endswith(".md") or filename >= os.path.basename(current_file):
        continue

    filepath = os.path.join(directory, filename)
    with open(filepath, "r") as f:
        lines = f.readlines()

    new_lines = []
    changed = False

    for line in lines:
        if line.lstrip().startswith("- [ ] "):
            task = line.strip()
            if task not in current_content and task not in pending_tasks:
                pending_tasks.append(task)
                origin_weeks.add(filename[:-3])
            changed = True
        else:
            new_lines.append(line)

    if changed:
        updates[filepath] = new_lines

if not pending_tasks:
    exit()

weeks_str = ", ".join(sorted(origin_weeks))
answer = input(f"Roll over {len(pending_tasks)} task(s) from {weeks_str}? (y/N): ")

if answer.strip().lower() != 'y':
    exit()

with open(current_file, "a") as f:
    if not os.path.exists(current_file):
        f.write(current_content)
    f.write(f"## Rolled Over\n")
    f.write("\n".join(pending_tasks) + "\n\n")

for filepath, lines in updates.items():
    with open(filepath, "w") as f:
        f.writelines(lines)
