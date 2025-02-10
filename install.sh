#!/bin/sh

sudo apt install git mosquitto mosquitto-clients

mkdir ~/.config/lxsession/
mkdir ~/.config/lxsession/LXDE-pi/
touch ~/.config/lxsession/LXDE-pi/autostart

echo "@lxpanel --profile LXDE-pi
@pcmanfm --desktop --profile LXDE-pi
@xscreensaver -no-splash
@point-rpi
@lxterminal
@leafpad
" >~/.config/lxsession/LXDE-pi/autostart

git clone https://github.com/CCC-Industry4/IIOT-4.0-Project ~/IIOT-4.0-Project

echo "xinput -set-prop \"10-0038 generic ft5x06 (79)\" \"Coordinate Transformation Matrix\" -1 0 1 0 -1 1 0 0 1
~/IIOT-4.0-Project/start.sh
" >~/.xsessionrc
