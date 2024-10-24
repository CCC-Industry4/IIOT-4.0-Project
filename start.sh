#!/bin/bash
#This script starts all necessary services to run the project along with the GUI

mosquitto&
node-red&

if [ -f /usr/local/ignition/gwcmd.sh ]; then 
	cd /usr/local/ignition && sudo ./ignition.sh start && ./node-red &
	while true; do
		if sudo /usr/local/ignition/gwcmd.sh -i | sed -n '4p' | grep 'RUNNING'; then
			sudo pkill chromium-browser
			DISPLAY=:0 chromium-browser --start-fullscreen http://localhost:8088/data/perspective/client/I4Project/smarthome &
			cd ~/IIOT-4.0-Project/ObjectDetection && ./start.sh
			sudo python ~/IIOT-4.0-Project/RaspiCam/camera.py &
			break
		fi
	done
else
	echo "Igntion is not installed"
fi
