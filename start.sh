#!/bin/bash
#This script starts all necessary services to run the project along with the GUI

mosquitto&
node-red
export DISPLAY=:1 && xinput set-prop 11 155 -1 0 1 0 -1 1 0 0 1
cd /usr/local/ignition && sudo ./ignition.sh start && ./node-red

#cd ~/IIOT-4.0-Project/GUI && ./IOT_GUI

