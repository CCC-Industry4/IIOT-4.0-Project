/*IOT Smart Home
see:https://www.keyestudio.com/products/keyestudio-esp32-smart-home-kit-for-esp32-diy-starter-kit-edu
Author: Matthew Graff /   Clovis Community College
Date: 12/13/2023
References:
- ESP32 Arduino Core: https://github.com/espressif/arduino-esp32
- WiFi Library: https://www.arduino.cc/en/Reference/WiFi
 */

#include <WiFi.h>          //Default Arduino library
#include <PubSubClient.h>  //Search PubSubClient By Nick O'Leary see:https://github.com/knolleary/pubsubclient and http://www.steves-internet-guide.com/using-arduino-pubsub-mqtt-client/
#include <Wire.h>          //Default Arduino library
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <ESP32Tone.h>
#include <analogWrite.h>
#include <ESP32Servo.h>
#include <MFRC522v2.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522Debug.h>
#include <SPI.h>
#include <Preferences.h>

int homeNumber = 0;
bool reset = false;
// Preferences
Preferences preferences;
//RFID
const uint8_t customAddress = 0x28;
TwoWire& customI2C = Wire;
MFRC522DriverI2C driver{ customAddress, customI2C };  // Create I2C driver.
MFRC522 mfrc522{ driver };                            // Create MFRC522 instance.
                                                      // Global variable to store UID
MFRC522::Uid storedUID;


#include "xht11.h"
xht11 xht(17);
Servo Wservo;
Servo Dservo;

#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C mylcd(0x27, 16, 2);

#define LED_PIN 26
// How many NeoPixels are attached to the Arduino?
#define LED_COUNT 4
// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

//Inputs
#define motionPin 14
#define pushbutton1Pin 16
#define pushbutton2Pin 27
#define touchPin 33
#define gasPin 23

//Output Pins
#define yellowLEDPin 12
#define buzzerPin 25
#define windowServo 5
#define doorServo 13

#define fanPin1 19
#define fanPin2 18

//#define homeNumber 2  ///////////////////////////////////////CHANGE THIS NUMBER FOR EACH HOME//////////////////////////////////////////////////

//const char* mqttnamespace = "Smart Homes Inc/Neighborhood 2";  //Each client needs to be different for one broker
                                                               //const char* client_name = "Classroom1/home2";  //Each client needs to be different for one broker
const char* ssid = "IT4Project";  // Replace the next variables with your SSID/Password combination
const char* password = "IOT12345";
const char* mqtt_server = "192.168.10.2";  //IP address of MQTT Broker - Raspberry Pi IP address
                                           //IPAddress static_ip(192, 168, 10, 177);   //might need to make ipaddresses static
String item = "0";

unsigned char dht[4] = { 0, 0, 0, 0 };  //Only the first 32 bits of data are received, not the parity bits

WiFiServer server(80);
bool config = false;
String readString; 
// Motor
int channel_PWM = 13;
int channel_PWM2 = 10;
int freq_PWM = 50;
int resolution_PWM = 10;
const int PWM_Pin1 = 5;
const int PWM_Pin2 = 13;

//Initialize NamespaceSmart Home Inc/Neighborhood #/
char client_topic[60];
char client_temperature[60];
char client_humidity[60];
char client_potentiometer[60];
char client_count[60];
char client_sensor1[60];
char client_sensor2[60];
char client_rfid[60];

char client_message[60];
char control1[60];
char control2[60];
char control3[60];
char control4[60];
char LEDcolorStrip[60];
char client_stepperSpeed[60];
char client_subscribe_all[60];


char client_motion[60];
char client_pushbutton1[60];
char client_pushbutton2[60];
char client_yellowLED[60];
char client_buzzer[60];
char client_gas[60];
char client_touch[60];

WiFiClient esp32Client;
PubSubClient client(esp32Client);


long lastMsg = 0;

char msg[50];
int value = 0;

int sensorValue = 0;
int sensorValueOld = -1;
String mes = "0";
String meas = "0";


float temperature = 0;
float oldTemperature = -1;
float humidity = 0;
float oldHumidity = -1;
int potentiometer = 0;
int old_potentiometer = -1;

String inputsNew = "test";
String inputsOld = "test2";
String message = "0";


bool motionOld = 1;
bool motionNew = 0;
bool gasOld = 0;
bool gasNew = 1;
bool pushbutton1Old = 0;
bool pushbutton1New = 1;
bool pushbutton2Old = 0;
bool pushbutton2New = 1;
int touchOld = -1;
int touchNew = 0;
String mqttNamespaceString;
void setup() {
  //set up serial
  Serial.begin(115200);

  preferences.begin("smarthome", false); 

  //setup LCD
  mylcd.init();
  mylcd.backlight();

  //set pins
  pinMode(yellowLEDPin, OUTPUT);
  pinMode(buzzerPin, OUTPUT);
  pinMode(motionPin, INPUT_PULLUP);
  pinMode(gasPin, INPUT_PULLUP);
  pinMode(pushbutton1Pin, INPUT_PULLUP);
  pinMode(pushbutton2Pin, INPUT_PULLUP);
  pinMode(touchPin, INPUT);

  pinMode(fanPin1, OUTPUT);
  pinMode(fanPin2, OUTPUT);

  //RFID
  mfrc522.PCD_Init();                                      // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);  // Show details of PCD - MFRC522 Card Reader details.
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));


  //setup LED
  ledcSetup(5, 1200, 8);      //Set the LEDC channel 1 frequency to 1200 and the PWM resolution to 8, that is, the duty cycle is 256.
  ledcAttachPin(fanPin2, 5);  //Bind LEDC channel 1 to the specified left motor pin gpio26 for output.

  //setup servo
  //ledcSetup(channel_PWM, freq_PWM, resolution_PWM);  //Set the servo channel, servo frequency, and PWM resolution.
  //ledcSetup(channel_PWM2, freq_PWM, resolution_PWM);
  //ledcAttachPin(PWM_Pin1, channel_PWM);   //Bind the LEDC channel to the specified IO port to achieve output
  //ledcAttachPin(PWM_Pin2, channel_PWM2);  //Bind the LEDC channel to the specified IO port to achieve output

  //Wservo.attach(windowServo);
  //Dservo.attach(doorServo);

  //  #if defined(__AVR_ATtiny85__) && (F_CPU == 16000000)
  //    clock_prescale_set(clock_div_1);
  //  #endif
  //    // END of Trinket-specific code.
  //  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  //  strip.show();            // Turn OFF all pixels ASAP
  //  strip.setBrightness(50); // Set BRIGHTNESS to about 1/5 (max = 255)

  //setup wifi
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
  Serial.println("TCP server started");
  MDNS.addService("http", "tcp", 80);
  mylcd.setCursor(0, 0);
  mylcd.print("ip:");
  mylcd.setCursor(0, 1);
  mylcd.print(WiFi.localIP());  //LCD display ip address of house

  //setup namespace
  /*
     sprintf(client_subscribe_all, "%s/#", mqttnamespace);
     sprintf(control1, "%s/control1", client_name);
     sprintf(control2, "%s/control2", client_name);
     sprintf(control3, "%s/control3", client_name);
     sprintf(control4, "%s/control4", client_name);
     sprintf(LEDcolorStrip, "%s/LEDcolorStrip", client_name);
     sprintf(client_motion, "%s/motion", client_name);
     sprintf(client_gas, "%s/gas", client_name);
     sprintf(client_pushbutton1, "%s/pushbutton1", client_name);
     sprintf(client_pushbutton2, "%s/pushbutton2", client_name);
     sprintf(client_touch, "%s/touch", client_name);
     sprintf(client_temperature, "%s/temperature", client_name);
     sprintf(client_humidity, "%s/humidity", client_name);
     sprintf(client_count, "%s/count", client_name);
  //sprintf(client_sensor1, "%s/in/sensor1", client_name);
  //sprintf(client_sensor2, "%s/in/sensor2", client_name);
  sprintf(client_message, "%s/out/message", client_name);
  //sprintf(client_yellowLED, "%s/out/yellowLED", client_name);
  //sprintf(client_buzzer, "%s/buzzer", client_name);
  sprintf(client_rfid, "%s/rfid", client_name);*/
  
  homeNumber = preferences.getInt("homeNumber", 0);
  reset = preferences.getBool("reset", false);
  String defaultNamespace = "";
  mqttNamespaceString = preferences.getString("namespace", defaultNamespace);
  const char* mqttnamespace = mqttNamespaceString.c_str();

  sprintf(client_subscribe_all, "%s/#", mqttnamespace);
  sprintf(control1, "%s/control1", mqttnamespace);
  sprintf(control2, "%s/control2", mqttnamespace);
  sprintf(control3, "%s/control3", mqttnamespace);
  sprintf(control4, "%s/control4", mqttnamespace);
  sprintf(LEDcolorStrip, "%s/LEDcolorStrip", mqttnamespace);
  sprintf(client_motion, "%s/motion", mqttnamespace);
  sprintf(client_gas, "%s/gas", mqttnamespace);
  sprintf(client_pushbutton1, "%s/pushbutton1", mqttnamespace);
  sprintf(client_pushbutton2, "%s/pushbutton2", mqttnamespace);
  sprintf(client_touch, "%s/touch", mqttnamespace);
  sprintf(client_temperature, "%s/temperature", mqttnamespace);
  sprintf(client_humidity, "%s/humidity", mqttnamespace);
  sprintf(client_count, "%s/count", mqttnamespace);
  sprintf(client_message, "%s/out/message", mqttnamespace);
  sprintf(client_rfid, "%s/rfid", mqttnamespace);

  if (!client.connected()) {
    reconnect();
    delay(500);
    message = String(0);
    client.publish(LEDcolorStrip, (char*)message.c_str());
    message = "null";
    client.publish(control1, (char*)message.c_str());
    client.publish(control2, (char*)message.c_str());
    client.publish(control3, (char*)message.c_str());
    client.publish(control4, (char*)message.c_str());
    Serial.println("intital values sent");
  }

  delay(500);
  mylcd.clear();
  mylcd.setCursor(0, 0);
  mylcd.print(("N" + mqttNamespaceString.substring(mqttNamespaceString.indexOf("Neighborhood ") + 13)).c_str());

  if (!reset || (!digitalRead(pushbutton1Pin) && !digitalRead(pushbutton2Pin))){
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Connect to URL:");
    mylcd.setCursor(2, 1);
    mylcd.println(WiFi.localIP().toString());
    server.begin();
    config = true;
  }

}

String test = "";
void loop() {
  if (config){
      // Create a client connection
      WiFiClient client = server.available();
      if (client) {
        while (client.connected()) {
          if (client.available()) {
            char c = client.read();

            //read char by char HTTP request
            if (readString.length() < 100) {

              //store characters to string 
              readString += c; 
              //Serial.print(c);
            } 

            //if HTTP request has ended
            if (c == '\n') {

              ///////////////
              Serial.println(readString); //see what was captured

              //now output HTML data header

              client.println("HTTP/1.1 200 OK");
              client.println("Content-Type: text/html");
              client.println();

              client.println("<HTML>");
              client.println("<HEAD>");
              client.println("<TITLE >Smart Home Configuration</TITLE>");
              client.println("</HEAD>");
              client.println("<BODY>");

              client.println("<H1 align='center'>Smart Home Configuration</H1>");

              client.println("<FORM ACTION='/' method=get align='center'>"); //uses IP/port of web page

              client.println("Enter new namespace name: <INPUT TYPE=TEXT NAME='NAMESPACE' VALUE='Smart Homes Inc/Neighborhood #/home#' SIZE='50' MAXLENGTH='50'>");
              client.println("<INPUT TYPE=SUBMIT NAME='submit' VALUE='Change'>");

              client.println("</FORM>");

              client.println("");
              client.println("<FORM ACTION='/' method=get align='center'>"); //uses IP/port of web page
              client.println("<INPUT TYPE=SUBMIT NAME='reset' VALUE='Finalize and Reset'>");
              client.println("</FORM>");            

              client.println("<FORM ACTION='/' method=get align='center'>"); //uses IP/port of web page
              client.println("<INPUT TYPE=SUBMIT NAME='default' VALUE='Reset to Default Settings'>");
              client.println("</FORM>");

              client.println("</BODY>");
              client.println("</HTML>");

              delay(1);
              //stopping client
              client.stop();

              /////////////////////
              Serial.println(readString);
              if(readString.indexOf("?NAMESPACE=") >0)//checks for on
              {
                String value = readString.substring(readString.indexOf("?NAMESPACE=") + 11, readString.indexOf("&"));
                value.replace("+", " ");
                value.replace("%2F", "/");
                value.replace("%23", "#");
                test = value;
                Serial.println(value);
              }
              if(readString.indexOf("default=Reset+to+Default+Settings") >0)//checks for off
              {
                preferences.putBool("reset", false);
                reset = false;
              }
              if(readString.indexOf("reset=Finalize+and+Reset") >0)//checks for reset
              {
                preferences.putInt("homeNumber", test.substring(test.indexOf("/home") + 5).toInt());
                preferences.putString("namespace", test);
                preferences.putBool("reset", true);
                reset = true;
                abort();
              }
              //clearing string for next read
              readString="";
            }
          }
        }
      }
    return;
  }

  if (!client.connected()) {
    reconnect();
    delay(500);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print(mqttNamespaceString.substring(mqttNamespaceString.indexOf("Neighborhood ") + 13).c_str());
    oldTemperature = -1;
    oldHumidity = -1;
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {  //update every second
    lastMsg = now;
    count();                 //increase count every second
    temperature_humidity();  //temprature & humidity
    touch();
  }

  gas();  //gas
  motion();
  pushbuttons();
  rfid();  //RFID
           //publish to MQTT
  /*
     if (rfid() == "114EB426") message = "1";  //LED strip red on
     if (rfid() == "FDC19162") message = "4";  //LED strip green on
     if (rfid() == "65B56175") message = "6";  //LED strip blue on
     if (rfid() == "E3B12BE8") message = "8";  //LED strip white on
     client.publish(LEDcolorStrip, (char*)message.c_str());                  //LED strip red on
   */
  /*
  //if (messageTemp.toInt() == 0) colorWipe(strip.Color(0, 0, 0), 50);      //LED strip off
  if (rfid() == "114EB426") colorWipe(strip.Color(255, 0, 0), 50);    //LED strip red on
                                                                      //if (messageTemp.toInt() == 2) colorWipe(strip.Color(200, 100, 0), 50);  //LED strip orange on
                                                                      //if (messageTemp.toInt() == 3) colorWipe(strip.Color(200, 200, 0), 50);  //LED strip yellow on
                                                                      if (rfid() == "FDC19162") colorWipe(strip.Color(0, 255, 0), 50);    //LED strip green on
                                                                                                                                          //if (messageTemp.toInt() == 5) colorWipe(strip.Color(0, 100, 255), 50);  //LED strip cyan on
                                                                                                                                          if (rfid() == "65B56175") colorWipe(strip.Color(0, 0, 255), 50);    //LED strip blue on
                                                                                                                                          if (rfid() == "E3B12BE8") colorWipe(strip.Color(255, 255, 255), 50);  //LED strip white on
   */
}

void setup_wifi() {
  delay(10);
  mylcd.clear();
  mylcd.setCursor(0, 0);
  //mylcd.print(String(client_name));
  //mylcd.print("/home" + homeNumber);
  mylcd.print("/home" + String(homeNumber));
  mylcd.setCursor(0, 1);
  mylcd.print("SSID: ");
  mylcd.print(String(ssid));
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  //WiFi.config(ip);
  WiFi.begin(ssid, password);
  if (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Connecting");
    mylcd.setCursor(0, 1);
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      mylcd.print(".");
    }
  }
  delay(1000);

  mylcd.clear();
  mylcd.setCursor(0, 0);
  mylcd.print("Wifi Connected!");
  mylcd.setCursor(0, 1);
  mylcd.print(WiFi.localIP().toString().c_str());
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);
}

//This function reads subscritptions
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println("Message arrived!!!");
  //Serial.print(client_message);

  Serial.print(topic);
  Serial.print(": ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT
  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off".
  // Changes the output state according to the message
  /*if (String(topic) == "client_topic/message") { //Subscribed to hello/sensor
    Serial.print("Recieving Data...");
    delay(500);
    if(messageTemp){ //Syntax if I wanna do something when a message appears
    }
    else{
    }
    }*/
  if (messageTemp == "true" || messageTemp == "True" || messageTemp == "1") {
    messageTemp = "1";
  } else if (messageTemp == "false" || messageTemp == "False" || messageTemp == "0") {
    messageTemp = "0";
  }
  if (String(topic) == control1) {
    digitalWrite(yellowLEDPin, messageTemp.toInt());
  }
  if (String(topic) == control2) {

    Dservo.attach(doorServo);
    if (messageTemp.toInt() == 1) Dservo.write(180);  //Open Door
    else Dservo.write(0);                             //Close Door
    delay(500);
    pinMode(doorServo, INPUT_PULLUP);  //must turn off because of interferance with buzzer
  }
  if (String(topic) == control3) {
    Wservo.attach(windowServo);
    if (messageTemp.toInt() == 1) Wservo.write(165);  //Open Window
    else Wservo.write(50);                            //Close Window
    delay(500);
    pinMode(windowServo, INPUT_PULLUP);  //must turn off because of interferance with buzzer
  }
  if (String(topic) == control4 && (messageTemp == "true" || messageTemp == "True" || messageTemp == "1")) {
    pinMode(buzzerPin, OUTPUT);
    tone(buzzerPin, 392, 250, 0);
    pinMode(buzzerPin, INPUT_PULLUP);  //must turn off because of interferance with buzzer
  }
  if (String(topic) == LEDcolorStrip) {
    if (messageTemp.toInt() == 0) colorWipe(strip.Color(0, 0, 0), 50);        //LED strip off
    if (messageTemp.toInt() == 1) colorWipe(strip.Color(255, 0, 0), 50);      //LED strip red on
    if (messageTemp.toInt() == 2) colorWipe(strip.Color(200, 100, 0), 50);    //LED strip orange on
    if (messageTemp.toInt() == 3) colorWipe(strip.Color(200, 200, 0), 50);    //LED strip yellow on
    if (messageTemp.toInt() == 4) colorWipe(strip.Color(0, 255, 0), 50);      //LED strip green on
    if (messageTemp.toInt() == 5) colorWipe(strip.Color(0, 100, 255), 50);    //LED strip cyan on
    if (messageTemp.toInt() == 6) colorWipe(strip.Color(0, 0, 255), 50);      //LED strip blue on
    if (messageTemp.toInt() == 7) colorWipe(strip.Color(100, 0, 255), 50);    //LED strip purple on
    if (messageTemp.toInt() == 8) colorWipe(strip.Color(255, 255, 255), 50);  //LED strip white on
    if (messageTemp.toInt() == 9) rainbow(10);                                //LED strip sfx1 on
    if (messageTemp.toInt() == 10) theaterChaseRainbow(50);                   //LED sfx2 on

    Serial.print(messageTemp.toInt());
  }
}

void reconnect() {
  while (!client.connected()) {                     // Loop until reconnected
    Serial.print("Attempting MQTT connection...");  // Attempt to connect
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Attempting MQTT");
    mylcd.setCursor(0, 1);
    mylcd.print("connection...");
    //if (client.connect(client_name)) {  //Client Name must be unique for every device in the network
    if (client.connect(("home" + String(homeNumber)).c_str())) {  //Client Name must be unique for every device in the network
      Serial.println("connected");
      // Subscribe
      client.publish(client_message, "Reconnected!");
      //client.subscribe(client_message);
      client.subscribe(client_subscribe_all);  //read all /out/#
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("MQTT Connected");
      mylcd.setCursor(0, 1);
      mylcd.print("W/");
      mylcd.print(String(mqtt_server));
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");  // Wait 5 seconds before retrying
      mylcd.clear();
      mylcd.setCursor(0, 0);
      mylcd.print("MQTT");
      mylcd.setCursor(0, 1);
      mylcd.print("Failed! Retrying...");
    }
  }
  }

  void colorWipe(uint32_t color, int wait) {
    for (int i = 0; i < strip.numPixels(); i++) {  // For each pixel in strip...
      strip.setPixelColor(i, color);               //  Set pixel's color (in RAM)
      strip.show();                                //  Update strip to match
      delay(wait);                                 //  Pause for a moment
    }
  }

  // Rainbow cycle along whole strip. Pass delay time (in ms) between frames.
  void rainbow(int wait) {
    for (long firstPixelHue = 0; firstPixelHue < 5 * 65536; firstPixelHue += 256) {
      for (int i = 0; i < strip.numPixels(); i++) {  // For each pixel in strip...
        int pixelHue = firstPixelHue + (i * 65536L / strip.numPixels());
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
      }
      strip.show();  // Update strip with new contents
      delay(wait);   // Pause for a moment
    }
  }

  // Rainbow-enhanced theater marquee. Pass delay time (in ms) between frames.
  void theaterChaseRainbow(int wait) {
    int firstPixelHue = 0;           // First pixel starts at red (hue 0)
    for (int a = 0; a < 30; a++) {   // Repeat 30 times...
      for (int b = 0; b < 3; b++) {  //  'b' counts from 0 to 2...
        strip.clear();               //   Set all pixels in RAM to 0 (off)
                                     // 'c' counts up from 'b' to end of strip in increments of 3...
        for (int c = b; c < strip.numPixels(); c += 3) {
          int hue = firstPixelHue + c * 65536L / strip.numPixels();
          uint32_t color = strip.gamma32(strip.ColorHSV(hue));  // hue -> RGB
          strip.setPixelColor(c, color);                        // Set pixel 'c' to value 'color'
        }
        strip.show();                 // Update strip with new contents
        delay(wait);                  // Pause for a moment
        firstPixelHue += 65536 / 90;  // One cycle of color wheel over 90 frames
      }
    }
  }

  void count() {

    value = value + 1;
    message = value;
    client.publish(client_count, (char*)message.c_str());
    mylcd.setCursor(11, 0);
    //mylcd.print("Count: ");
    mylcd.print(message);  //Display Count
    Serial.print(client_count);
    Serial.print(": ");
    Serial.println(message);
  }

  void temperature_humidity() {
    if (xht.receive(dht)) {  //Returns true when checked correctly
                             //temperature = dht[2];  //The integral part of temperature, DHT [3] is the fractional part
      temperature = dht[2] + dht[3] / 10.0;  //The integral part of temperature, DHT [3] is the fractional part
      if (temperature != oldTemperature) {
        Serial.print("Temp:");
        Serial.print(temperature);
        Serial.println("C");
        mylcd.setCursor(0, 1);
        mylcd.print(temperature, 1);
        mylcd.print((char)223);
        mylcd.print("C ");
        message = String(temperature);
        client.publish(client_temperature, (char*)message.c_str());
        oldTemperature = temperature;
      }
      //humidity = dht[0];  //The integral part of Humidity, DHT [1] is the fractional part
      humidity = dht[0] + dht[1] / 10.0;  //The integral part of Humidity, DHT [1] is the fractional part
      if (humidity != oldHumidity) {
        Serial.print("Humidity:");
        Serial.print(humidity);
        Serial.println("%");
        mylcd.setCursor(8, 1);
        mylcd.print(humidity, 1);
        mylcd.print("% ");
        message = String(humidity);
        client.publish(client_humidity, (char*)message.c_str());
        oldHumidity = humidity;
      }
    } else {  //Read error
      Serial.println("sensor error");
    }
  }

  void motion() {
    motionNew = digitalRead(motionPin);
    if (motionOld != motionNew) {
      message = String(motionNew);
      client.publish(client_motion, (char*)message.c_str());
      Serial.print(client_motion);
      Serial.print(": ");
      Serial.println(message);
      motionOld = motionNew;
    }
  }

  void gas() {
    gasNew = digitalRead(gasPin);
    if (gasOld != gasNew) {
      message = String(gasNew);
      client.publish(client_gas, (char*)message.c_str());
      Serial.print(client_gas);
      Serial.print(": ");
      Serial.println(message);
      gasOld = gasNew;
    }
  }

  void pushbuttons() {
    pushbutton1New = digitalRead(pushbutton1Pin);
    if (pushbutton1Old != pushbutton1New) {
      message = String(pushbutton1New);
      client.publish(client_pushbutton1, (char*)message.c_str());
      Serial.print(client_pushbutton1);
      Serial.print(": ");
      Serial.println(message);
      pushbutton1Old = pushbutton1New;
    }

    pushbutton2New = digitalRead(pushbutton2Pin);
    if (pushbutton2Old != pushbutton2New) {
      message = String(pushbutton2New);
      client.publish(client_pushbutton2, (char*)message.c_str());
      Serial.print(client_pushbutton2);
      Serial.print(": ");
      Serial.println(message);
      pushbutton2Old = pushbutton2New;
    }
  }

  void touch() {
    touchNew = touchRead(touchPin);
    if (touchOld != touchNew) {
      message = String(touchNew);
      client.publish(client_touch, (char*)message.c_str());
      Serial.print(client_touch);
      Serial.print(": ");
      Serial.println(message);
      touchOld = touchNew;
    }
  }

  String rfid() {
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
      return "null";
    }

    // Save the UID
    storedUID = mfrc522.uid;

    // Clear the previous UID value
    message = "";

    // Construct UID value as a string
    for (byte i = 0; i < storedUID.size; ++i) {
      message += (storedUID.uidByte[i] < 0x10 ? "0" : "");
      message += String(storedUID.uidByte[i], HEX);
    }

    // Convert the entire string to uppercase
    message.toUpperCase();

    //publish to MQTT
    client.publish(client_rfid, (char*)message.c_str());

    // Print UID to Serial
    Serial.print(F("UID Value: "));
    Serial.println(message);
    return message;
  }
