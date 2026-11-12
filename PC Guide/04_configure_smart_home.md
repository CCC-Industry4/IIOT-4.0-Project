[Back to Main Menu](https://github.com/CCC-Industry4/IIOT-4.0-Project/tree/main)

# Configure Smart Home
---
1. Open up an instance of PowerShell on your Windows machine and type in the command: ipconfig. Locate your IPv4 address and save it for later. In the example image provided below the IPv4 address is 192.168.10.23. ![Ipconfig](./images/ipconfig.png)
2. Plug in and power up a Smart Home (make sure it has access to sufficient power, problems can occur when the house doesn't have enough power).
3. Set the home into config mode, if you are unsure how to do this you can reference the PowerPoint guide or continue with this written one.
     1. Look inside the smart home and locate the reset button
     2. press the reset button and pay attention to the on-screen instructions
     3. it should say press both buttons to enter config mode, if you missed the prompt you can go back to step 2 or follow the new on-screen instructions
4. Now that the smart home is in config mode it has become a Wi-Fi access point that you can connect to with a Wi-Fi supported device such as your phone or your pc. While connected to Config_ESP32, navigate to the IP listed on the LCD screen using a web browser.
      ![Smart home Configuration page](./images/smartHomeConfig.png)

  -  Ask your instructor for the neighborhood and home number:
      -  Neighborhood number(s) -**should** be the same for the whole class.
      -  Home numbers **must** be unique for each home in the neighborhood.
      -  MQTT IP will be the IPv4 number that you should have from earlier
      -  Wi-Fi SSID and Pass will be any Wi-Fi you can connect to, but you **must** be connected to the same Wi-Fi as your pc and be connected to the same Wi-Fi for any device that you want to interact with ignition
      -  You can add rules or key fobs now if you wish to do so.
	  - **Click Finalize and Reset ESP32.** When you are ready.

5. **Verify** that your Smart Home is working, while connected to this [link](http://localhost:8088) click on home and under the perspective category click launch, click I4Project_Demo. Here you can play around with different smart home functions and make sure they are working properly. You can also grab the link at the top of your screen and replace "localhost" with your IPv4 address and use that link on another device that is connected to the **same** Wi-Fi as your main PC. ![Ignition Perspective](./images/perspective.png)

6. You can not have two smart homes with the same namespace.  If your home doesn’t connect, look at the namespace of a working Smart Home. 
