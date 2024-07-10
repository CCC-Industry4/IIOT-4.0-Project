# IIOT-4.0-Project ;)

__Sponsored by the National Science Foundation (Award Number: 2202201)__

__by: Matthew Graff, Neiro Cabrera, William (Bill) Kerney, Meagan Eggert__

__From: Clovis Community College in Fresno, California__

This project's purpose is to bring exposure to concepts regarding Fourth Industrial Revolution and Industry 4.0 at minimal cost. This is specifically designed for demonstrations at the High School level and more in depth training at the college level. Additionally, the demo serves as a great starting point for anyone interested in learning on a functioning prototype.

This repo will contain steps to set up a system to create an Internet of Things (IOT) using Arduino ESP32s as clients and a [Raspberry Pi](https://www.raspberrypi.org/) as an MQTT Broker using [Mosquitto](https://mosquitto.org/). It will also include a GUI designed for reading the data being published on this project using [dearImgui](https://github.com/ocornut/imgui). It also uses a SCADA system called [Ignition by Inductive Automation](https://inductiveautomation.com/ignition/) to view the data from the MQTT broker as well. This README will explain how to recreate this demo using all the parts described above.

## Getting Started

1. Begin by cloning this repo in your Linux terminal: ```git clone https://github.com/neiroc-02/IIOT-4.0-Project```
2. After it finishes cloning, enter the directory using: ```cd IIOT-4.0-Project```
3. Run the some_installs.sh script to download dependencies: ```./some_installs.sh```

After these steps, all the dependencies for Mosquitto and dearImgui are downloaded. Now you can begin working on building the rest of the project.

## Some Vocab

In Progress...

## Mosquitto - MQTT Broker on Raspberry Pi

![th](https://github.com/neiroc-02/IIOT-4.0-Project/assets/113735504/59e3cf17-bcc4-4483-960a-695b1663d10c)

Helpful Link -> https://www.switchdoc.com/2018/02/tutorial-installing-and-testing-mosquitto-mqtt-on-raspberry-pi/

The Raspberry Pi will serve as the broker that controls and distributes the data between the Arduino and the GUI (and a SCADA system like Ignition as well). This will allow for data to be sent between devices. Some important commands to know are as follows:

  ```mosquitto or sudo /etc/init.d/mosquitto start ``` - Starts the Broker
  
  ```sudo /etc/init.d/mosquitto stop               ``` - Stops the Broker
  
  ```mosquitto_sub -d -t "{namespace}"             ``` - Subscribes you to a namespace 
  
  ```mosquitto_pub -d -t "{namespace}" -m "{data}" ``` - Sends data to the namespace specified

This serves as the heart of the demo. Mosquitto (MQTT) allows for the communication between smart devices. This uses the unified namespace to set a naming scheme between different data sets. If you would like to send a message to line1/thing1/out/message from the terminal, the mosquitto command would equate to…

```mosquitto_pub -d -t “line1/thing1/out/message” -m “Hello World!”```

Before you use the broker, you must edit your /etc/mosquitto/mosquitto.conf file. In the end, your mosquitto.conf file should look like this...

```
pid_file /run/mosquitto/mosquitto.pid

persistence true
persistence_location /var/lib/mosquitto/

log_dest topic

log_type error
log_type warning
log_type notice
log_type information

connection_messages true
log_timestamp true
allow_anonymous true
listener 1883

include_dir /etc/mosquitto/conf.d
```

Our Arduino and GUI code utilizes this functionality to communicate between everything involved in our network. The applications become clearer as you continue to read through our demo. 

### Crontab (Optional)

Helpful Link -> https://www.tomshardware.com/how-to/run-script-at-boot-raspberry-pi

If you would like the Moquitto Broker on your Pi to start at boot, you can use crontab by following these steps:
1. Type this in your terminal:       ```crontab -e```
2. Inside your crontab, type this:   ```@reboot mosquitto```
3. Exit and save the crontab and then the broker should start on boot.

## The Router

Helpful Link -> https://www.tomshardware.com/how-to/static-ip-raspberry-pi

In order to run the project, you must have a router to avoid firewalls. The way we chose to do this was to buy a cheap router and configure it to this setting:

Name:     ```IT4 Project```

Password: ```IOT12345```

The Raspberry Pi must also be configured with a static IP address. You can either do that through the router or dhcpcd.conf file. A helpful article for the second method is included in the link above.

## The ESP32

![th](https://github.com/neiroc-02/IIOT-4.0-Project/assets/113735504/6c1db13e-7317-4324-9925-5e29fd4c2c71)

Helpful Link -> https://randomnerdtutorials.com/esp32-mqtt-publish-subscribe-arduino-ide/

This demo utilizes the Arduino ESP32 since it has WiFi capabilities that allow it to connect to a network where smart devices can communicate with the broker. This library uses Wifi.h to connect to the same network as the broker. This allows the ESP32 to communicate and send its data through MQTT to the broker. 

### The PCB (Optional)

![PCB](https://github.com/neiroc-02/IIOT-4.0-Project/assets/113735504/bab3020f-0d90-43dc-a1ba-198fa9198705)

The PCB uses an ESP32 and serves as a hub for data to be published and sent to the MQTT Broker. We are currently updating the PCB after Version 1.0. The old version is posted but there are a few bugs that need to fixed. If anyone notices them, please post your thoughts on the issues section.

## The GUI

DearImgui Github -> https://github.com/ocornut/imgui

The GUI serves to display the data from the broker in an easy to visualize manner. This displays the power of Internet of Things systems without having to learn a SCADA system like Ignition. DearImgui is a C++ based Graphical User Interface Library that allows for the creation of windows that can display information or graphs. This allows us to display all the data being circulated in our network between the Arduino, Pi, Ignition (maybe), and the GUI itself. DearImgui and its dependencies are automatically downloaded when you run the script ```some_installs.sh```.

![GUI](https://github.com/neiroc-02/IIOT-4.0-Project/assets/113735504/7854474c-ead8-4735-a595-4c0e23f0556e)

In order to run the GUI, first compile the code using ```make``` then run the executable ```IOT_GUI```. If you would like to edit the GUI, you may change main.cpp to fit your personal needs.

### Read Library

Read.h Github -> https://github.com/ShakaUVM/read

The Read library is used to simplify IO and make parsing mosquitto messages easier. The read library combines cin and vetting to allow us to write simpler code in fewer lines. It will be automatically downloaded in your ```/usr/include``` when you run the ```some_installs.sh``` script for easy access in this repo and any other project you may deem it useful.

## Ignition

Ignition serves as a SCADA system to help visualize our project with or without the GUI. Ignition has more industrial uses as a SCADA system companies use to manage data from smart devices in their IOT networks. We can visualize the project using the Ignition Designer and the image below shows what the project looks like when you import the project, tags, and picture from the Ignition directory into your Designer.

![Ignition Perspective](https://github.com/neiroc-02/IIOT-4.0-Project/assets/140917053/6f35b826-1244-40c7-8dd2-d0304887d41d)

### Downloading Ignition Gateway on the Pi

You can run the Ignition Gateway on a Raspberry Pi and use Ignition Designer. Download the Gateway on the Pi and PC.  Then run the Designer on your PC to view the project we have. See the [Ignition Quick Start Guide.](https://docs.inductiveautomation.com/display/DOC80/Quick+Start+Guide) The steps are as follows:

1. Download Ignition [here](https://inductiveautomation.com/downloads/ignition/8.1.31) and be sure to chose the correct one for your Operating System. If you run a Raspberry Pi OS on a 64 bit Architecture, you should choose __Ignition - Linux AARCH64 zip__.
2. After your Download finishes, open your terminal and enter the Downloads folder using ```cd Downloads```.
3. Next, run the following command to unzip the downloaded folder: ```sudo unzip {name of Downloaded file} -d /usr/local/ignition```
4. After the folder finishes unzipping, enter the new directory: ```cd /usr/local/ignition/```
5. Next, run this command to make all necessary files executable: ```sudo chmod +x *.sh ignition-gateway```
6. Then run the now exectuable Ignition script: ```sudo ./ignition.sh start```. Congratulations! The Ignition Gateway has started.
7. Now, enter your browser and run the local host: localhost:8088. Give the Gateway a few minutes to start.
8. The first time you open it, it will ask you which form of Ignition you want to use. Choose the one that best fits your needs, but we chose Full Ignition when building this project.
9. Now Ignition is running on the Raspberry Pi!


Now that Ignition is downloaded, you can start ignition any time from the terminal by entering ```cd /usr/local/ignition``` then running the script ```sudo ./ignition.sh start```. Then open the gateway by entering in your browser __localhost:8088__ and you will be able to view the gateway!

### Installing MQTT Modules on Ignition
To install the MQTT modules for Ignition, you must do the following (see this ![video](https://inductiveautomation.com/resources/video/mqtt-distributor-module) for more information):
1. Use this [link](https://inductiveautomation.com/downloads/third-party-modules/8.1.32) and locate the MQTT Distributor, Transmission, and Engine modules. Download all three and then from your Ignition Gateway you should be able to install the modules using the ```*.modl``` files you downloaded.
2. Open the MQTT server using the web browser on the Pi.In a web browser, type http://localhost:8088 to the ignition server.
3. Go to Config/Sytem/Modules then locate "Install or Upgrade a Module..." at the bottom of the page.
4. Now you will see these modules listed at the bottom of the Config bar.

### Configuring MQTT Engrine Module on Ignition
1. Click on Config/MQTT Engine/Settings.
2. In the General Tab unselect: "Block outbound edge node tag writes" and "Block outbound device tag writes."  Then save changes.
3. Under Servers "Create new MQTT Server Settings.  Name new Server "PI" (so that this matches other files you download). Add URL (i.e. tcp://192.168.10.2.) Then click "Create New MQTT Server Settings."
4. Under Namespace/Custom give a name and put the # sound to subscribe to everything, then save changes.

### Ignition Licenses for Education
Inductive Automation partners with qualifying educational institutions to provide free Ignition licenses to students anywhere in the world!  Apply here: https://inductiveautomation.com/educational-engagement

### Object Detection using YOLOv5
1. To install YOLOv5 in the repository run `./ObjectDetection/install.sh`
2. To run it use `./ObjectDetection/start.sh`
*add more details later*

### Training YOLOv5
1. Clone the (labelImg repository)[https://github.com/HumanSignal/labelImg] and the YOLOv5
