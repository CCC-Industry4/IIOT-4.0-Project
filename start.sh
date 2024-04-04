#!/bin/bash
#This script starts all necessary services to run the project along with the GUI

mosquitto&

cd /usr/local/ignition && sudo ./ignition.sh start

#cd ~/IIOT-4.0-Project/GUI && ./IOT_GUI

cd ~/IIOT-4.0-Project/ObjectDetection && ./start.sh
