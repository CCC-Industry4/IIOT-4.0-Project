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
@~/test.sh
@~/thing.sh" >~/.config/lxsession/LXDE-pi/autostart

git clone https://github.com/CCC-Industry4/IIOT-4.0-Project ~/IIOT-4.0-Project
