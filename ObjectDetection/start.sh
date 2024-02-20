#!/usr/bin/bash
# start motion

# activate virtual environment
source ./.venv/bin/activate
python3 ./detect.py &
