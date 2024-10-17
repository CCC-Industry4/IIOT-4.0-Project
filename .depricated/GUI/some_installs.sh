#!/bin/bash

#These are all the packages you must install for the project.
#Most are for the GUI, mosquitto is what you use to set up the broker.

#Installs necessary packages for linux systems (using Debian)
packages=("mosquitto" "mosquitto-clients" "libglfw3-dev" "libglfw3" "xorg-dev" "libxcb1-dev")
for str in ${packages[@]}; 
do
	sudo apt install $str
done

#Clones necessary github repos in /usr/include
githubs=("https://github.com/ocornut/imgui" "https://github.com/ShakaUVM/read")
for str in ${githubs[@]}; 
do
	cd /usr/include/ && sudo git clone $str
done

#Downloading a header file from this repo: https://github.com/nothings/stb.git
sudo curl -o /usr/include/stb_image.h https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
