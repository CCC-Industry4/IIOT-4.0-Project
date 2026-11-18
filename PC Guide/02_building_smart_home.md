[Back to Main Menu](https://github.com/CCC-Industry4/IIOT-4.0-Project/tree/main)

# Building a Smart Home

---

1. **Instructions for building the Smart Home:**
   - See the [Building Guide](https://github.com/CCC-Industry4/StarterGuide/blob/8cd98f201a4158275c46c4c5b58a1fdbb2670bd0/Building.md) (__Highly Recommended__)
   - See the official [Keyestudio Smart Home Guide](https://docs.keyestudio.com/projects/KS5009/en/latest/docs/index.html) (__Deprecated__)
        1. *Keep in mind that the Keyestudio Guide incorrectly wired the Steam Sensor to IO34, it should be wired to IO33.*
    
2. **Pin Connections:**

    - See the [Wiring Guide](https://github.com/CCC-Industry4/StarterGuide/blob/8cd98f201a4158275c46c4c5b58a1fdbb2670bd0/Wiring.md)

   Connect the following components to the corresponding pins on your ESP32 board:

   - **Temperature and Humidity Sensor**: Pin **IO17**
   - **Yellow LED Module**: Pin **IO12**
   - **Steam Sensor (used as touch sensor)**: Pin **IO33**
   - **Fan**: **IN-** to **IO18**, **IN+** to **IO19**
   - **PIR Motion Sensor**: Pin **IO14**
   - **Left Button Module**: Pin **IO16**
   - **Right Button Module**: Pin **IO27**
   - **RFID Module**: **BUS I2C**
   - **LCD1602 Display**: **BUS I2C**
   - **6812 RGB LED**: Pin **IO26**
   - **Gas Sensor**: Pin **IO23**
   - **Buzzer Sensor**: Pin **IO25**
   - **Servo (Windows)**: Pin **IO5**
   - **Servo (Doors)**: Pin **IO13**

---
