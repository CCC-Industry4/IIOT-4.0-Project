# !/usr/bin/bash
# Use this script if any libraries are missing from the virtual environment
source ./.venv/bin/activate

python3 -m pip install torchvision flask paho.mqtt opencv-python configparser
