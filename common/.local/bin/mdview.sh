#!/bin/bash
# set -x

# Configuration
MARGIN="1in" 
ENGINE="xelatex" 

# Input validation
if [[ -z "$1" ]]; then
    exit 1
fi

INPUT_FILE="$(readlink -f "$1")"
TEMP_PDF="/tmp/$(basename "${INPUT_FILE%.*}").pdf"

# Function to run the watcher
start_watcher() {
    # -n: non-interactive
    # -r: reload if file is replaced (better for atomic saves)
    # Redirect all output to /dev/null
    while true; do
        echo "$INPUT_FILE" | entr -r pandoc "$INPUT_FILE" \
            -V geometry:margin="$MARGIN" \
            --pdf-engine="$ENGINE" \
            -o "$TEMP_PDF" >/dev/null 2>&1
        sleep 1
    done
}

# 1. Initial manual convert to ensure file exists
pandoc "$INPUT_FILE" -V geometry:margin="$MARGIN" --pdf-engine="$ENGINE" -o "$TEMP_PDF" >/dev/null 2>&1

# 2. Start the watcher in the background and disown it
start_watcher & 
disown

# 3. Open Zathura in the background and disown it
zathura "$TEMP_PDF" >/dev/null 2>&1 &
disown

# 4. Exit the script immediately
exit 0
