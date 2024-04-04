#!/usr/bin/bash
# start motion
#if pgrep -x motion > /dev/null 
#then
#	echo "Motion is running"
#else
#	echo "Starting Motion"
#	sudo motion
#fi

# activate virtual environment
source ./.venv/bin/activate
python3 ./detect.py &
