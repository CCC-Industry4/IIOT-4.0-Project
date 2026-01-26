# CCC Industry 4.0 Instructor Training
---
## Sponsored by the National Science Foundation (Award Number: 2202201)
**Instructors:** Matthew Graff and William (Bill) Kerney  
**Student Workers:** Neiro Cabrera, Meagan Eggert, Julian Perry Laxamana, Gurkaran Singh, Mohammad Abumaali, Benjamin Hallaway  :)
**School:** Clovis Community College in Fresno, California

## Project Description
The purpose of this project is to introduce concepts related to the Fourth Industrial Revolution and Industry 4.0 at minimal cost. It is tailored for demonstrations at the high school level and offers more in-depth training opportunities at the college level. Moreover, this demo serves as an ideal starting point for anyone interested in learning through hands-on experience with a functional prototype.  

This repository will provide instructions for setting up a system to create an Internet of Things (IoT) using Arduino ESP32s as clients and a [Raspberry Pi](https://www.raspberrypi.org/) as an MQTT Broker, integrated with SCADA using [Ignition by Inductive Automation](https://inductiveautomation.com/ignition/). The README will guide you through recreating this demonstration using the specified components, followed by a brief tutorial.
![thumbnail_IMG_3054](https://github.com/user-attachments/assets/62ca498c-cfb3-4ad6-86ff-0716ecaad7cd)


## Kit Components To Purchase
The following are the components used in this project.  Depending on the situtation different components maybe used to replace the following.
- Raspberry Pi 4 or Pi 400 - $100

   <img src="https://github.com/user-attachments/assets/4806f096-aa6b-4e5b-b601-6692c7f613c4" style="width: 20%;" alt="71A14Sz2bWL _AC_SL1500_">


- Arduino ESP43 - [KEYESTUDIO ESP32 Smart House](https://www.keyestudio.com/products/keyestudio-esp32-smart-home-kit-for-esp32-diy-starter-kit-edu) - $55

<img src="https://github.com/user-attachments/assets/3cebbbc6-9c92-4065-af2b-c9d46f695485" style="width: 20%;" alt="71aY7HXNc5L _AC_SL1500_">

- Router [GL.iNet GL-AR300M16-Ext](https://store-us.gl-inet.com/products/gl-ar300m16-mini-smart-router) or most any router - $30

<img src="https://github.com/user-attachments/assets/8f36c3b7-f8ef-4a87-8133-f80b9922bc74" style="width: 20%;" alt="41gQQAA8ozL _AC_SL1000_">

  
Optional:
- Touchscreen Raspberry Pi 7" Touch Screen Display - $75
- [Case SmartiPi Touch 2](https://www.adafruit.com/product/4377?gad_source=1&gclid=CjwKCAjwqMO0BhA8EiwAFTLgIMRqNCXHAhp_l-yysnQuAGzoeCjPC7tV8XhQrg3Q21p99cGYjvx5rBoCb64QAvD_BwE) - $35
 
  <img src="https://github.com/user-attachments/assets/3f5fcbb1-25a9-42d6-ae0f-e2ca17844d25" style="width: 20%;" alt="4377-04">
- Opto 22 [GRV-RIO-LC](https://www.opto22.com/products/product-container/grv-rio-lc) - $1,095

  *This system is for more advanced training for college programs.  This system shows how industry deploys Industry 4.0 technology.*
  
![image](https://github.com/user-attachments/assets/baf884df-4738-427b-814a-6525a1b212ee)

---
## Table Of Contents

To install the project, run the following command in Raspberry Pi terminal with WiFi connection:
```
   curl -o- https://raw.githubusercontent.com/CCC-Industry4/IIOT-4.0-Project/main/install.sh | bash -x
```

### If you are starting with unconfigured components…
- [Initialize Router](https://github.com/CCC-Industry4/StarterGuide/blob/main/01_initializing_router.md)  
- [Building Smart home](https://github.com/CCC-Industry4/StarterGuide/blob/main/02_building_smart_home.md)
- [Initialize Raspberry Pi](https://github.com/CCC-Industry4/StarterGuide/blob/main/03_initialize_raspberry_pi.md)
### Industry 4.0 Project System Laboratory Exercises 
- [Lab 1. Set-up Raspberry Pi, Arduino Smart Home, and PC](https://github.com/CCC-Industry4/StarterGuide/blob/main/04_setup_raspberry_pi.md)
- [Lab 2a. Configure camera object detection](https://github.com/CCC-Industry4/StarterGuide/blob/main/05_configure_camera.md)
- [Lab 2b. Configure Smart Home](https://github.com/CCC-Industry4/StarterGuide/blob/main/06_configure_smart_home.md)
- [Lab 3. Operation Smart Home Neighborhood](https://github.com/CCC-Industry4/StarterGuide/blob/main/07_operation_smart_home_neighborhood.md)
- [Lab 4. Troubleshooting](https://github.com/CCC-Industry4/StarterGuide/blob/main/08_troubleshooting.md)
- [8 Labs on creating a Printed Circuit Board](https://docs.google.com/document/d/13rz4dXHV7b0cbep-HrDET545i4Urjr8j9SfGMteeJzM/edit?usp=sharing) The PCB can be used to creat custom boards for the ESP32.

### Training Materials
 - [Indutry 4.0 PowerPoint](https://docs.google.com/presentation/d/1RTdsWulPext4mxPflkVPRlp7vQk2u_fBCdiXWWHHBMw/edit?usp=sharing)
---
