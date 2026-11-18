[Back to Main Menu](https://github.com/CCC-Industry4/IIOT-4.0-Project/tree/main)

# Troubleshooting
Ethernet - Verify settings using Windows PC using ethernet cable
1. Search “Ethernet settings” using Windows search bar.
2. Double-click on Change adapter options.
   ![Ethernet Settings](https://github.com/user-attachments/assets/405834df-6ddc-40ef-93c7-ccc00f8f6d7a)
   If you do not see this option, instead search "View network connections" in the Windows search bar. Then continue following the rest of the steps.
   

<p align="center">
  <img src="https://github.com/CCC-Industry4/IgnitionProject/raw/Cane/images/troubleshooting/TroubleshootingEthernet1.png" width="45%" />
  <img src="https://github.com/CCC-Industry4/IgnitionProject/raw/Cane/images/troubleshooting/TroubleshootingEthernet2.png" width="45%" />
</p>



3. Double click on Ethernet.
4. Click on Properties.
   
   ![Ethernet Status](https://github.com/user-attachments/assets/e105e93d-9ab3-4ca1-9910-3e4b6637564c)

5. Double click on Internet Protocol Version 4 (TCP/IPv4).

   ![Ethernet Prooperties](https://github.com/user-attachments/assets/62cd93aa-09f8-46f5-a0b1-928f7255cc72)

6. Verify that “Obtain an IP address automatically” is selected.

    ![Internet Protocol Version 4 (TCP-IPv4) Properties](https://github.com/user-attachments/assets/4287082b-46f7-495e-994e-c55a2cc08b14)

7. Restore Session by pressing button or use following link: http://192.168.10.2:8088/data/perspective/client/I4Project/smarthome 


Ethernet - Verify connectivity and data transfer

1. The image illustrates data transfer between applications using Ethernet. For more details, refer to the source website (https://iebmedia.com/technology/looking-behind-the-automation-protocols/). This chart is also valuable for troubleshooting, pinpointing where issues may arise. Troubleshooting starts from the physical layer at the bottom and progresses upward through the application layer.

![ieb23protocol1](https://github.com/user-attachments/assets/a2bcc21b-4e64-4a1d-9d79-986144985ba6)

2. Open the Command Prompt Window.  If you have a hard time locating the Command Prompt Window search ‘CMD’ in the Windows search bar.  
   -If you have questions about specific commands you hand type the command listed followed by `-help`.
   -To stop a command from running, press `Ctrl + C`.

3. Physical Layer- To ensure the Physical Layer is working:
   -Verify that the Ethernet cable is securely plugged in.
   -Check that the link status LEDs on the network interface are illuminated or flashing.
   -If using Wi-Fi, ensure that the device is connected to the desired SSID (e.g., IOTProject).

4. Data Link Layer- To test the Data Link Layer:
   Use commands like `ipconfig` (or `ifconfig` on Linux or Raspberry Pi) to inspect network interface configurations.

      ![ipconfig](https://github.com/user-attachments/assets/c637d53e-4d65-4254-b4ed-b54ce11bf2d1)

      Please note that under Ethernet, this computer is connected with an IPv4 address of 192.168.10.160. Each computer will receive a unique IP address. Check     your computer's IP address, which should start with 192.168.10..., but the last three numbers will vary. If you are connected via Wi-Fi to the system, check under the Wi-Fi adapter. In the example above, the Wi-Fi is connected to the internet, not to the local system.

   -Use the `ping` command to verify connectivity with other devices on the network.  In the case of our system the following devices have static IP addresses:
      Device         IP Address
      Router         192.168.10.1
      Raspberry PI   192.168.10.2
      Camera         192.168.10.3

   -To test connection to the router type  `ping 192.168.10.1`.
      
      ![ping](https://github.com/user-attachments/assets/6e43d7bf-91d9-4161-adcc-01e02244d73c)
   
   -Test connections with other items in the system.
   -Try pinging any random ip address starting with the same ‘192.168.10.’ ending with any number from 1-255.  What do you see happen?

5. Network Layer- The Network Layer manages logical addressing and routing:
   -Verify routing tables using the `route` command on Windows or `netstat -r` on Linux. Routing tables contain information about network paths and direct traffic to its destination efficiently. Checking these tables helps diagnose connectivity issues and ensures proper routing configuration across the network infrastructure.  

6. Transport Layer- The Transport Layer ensures reliable data transfer between devices:
   -Use utilities like `netstat` or `ss` to check TCP and UDP connections. To stop a command from running, press `Ctrl + C`.
   -Test TCP connectivity with tools like Telnet or SSH, and UDP with tools like `nc` (netcat).

7. Application Layer- The Application Layer supports end-user applications and services:
   -Check application-specific settings and configurations (e.g., web browser settings for HTTP).
   -Use application-specific troubleshooting tools or logs (e.g., web server access logs).
   -In the case of the I4.0 Project use the Ignition SCADA to verify functionality.
