// Libraries
#include <WiFi.h>       
#include <PubSubClient.h>  
#include <Wire.h>       
#include <ESPmDNS.h>
#include <Adafruit_NeoPixel.h>
#include <Buzzer.h>
#include <ESP32Servo.h>
#include <MFRC522v2.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522Debug.h>
#include "xht11.h"
#include <SPI.h>
#include <Preferences.h>
#include <LiquidCrystal_I2C.h>

// Configuration
// *CHANGE THESE
const char* ssid = "IT4Project";
const char* password = "IOT12345";
const char* mqtt_server = "192.168.10.2";

// Set model
//#define SMARTHOME
#define SMARTFARM

// End of Config

// Import Pins
#include "pins.h"

// Initialize Namespace
// controls
static char control1[100];
static char control2[100];
static char control3[100];
static char control4[100];
static char control5[100];
static char LEDcolorStrip[100];

static char client_topic[100];
static char client_temperature[100];
static char client_humidity[100];
static char client_count[100];
static char client_sensor1[100];
static char client_sensor2[100];
static char client_rfid[100];
static char client_message[100];
static char client_stepperSpeed[100];
static char client_subscribe_all[100];
static char client_motion[100];
static char client_pushbutton1[100];
static char client_pushbutton2[100];
static char client_yellowLED[100];
static char client_buzzer[100];
static char client_gas[100];
static char client_touch[100];
static char client_water[100];
static char client_soil[100];

bool reset = false;
int homeNumber = 5;
Preferences preferences;

// LED STRIP
#ifdef LEDStripPin
Adafruit_NeoPixel strip(LED_COUNT, LEDStripPin, NEO_GRB + NEO_KHZ800);
#endif

// LCD Screen
LiquidCrystal_I2C mylcd(0x27, 16, 2);

// RFID
#ifdef RFID
const uint8_t customAddress = 0x28;
TwoWire& customI2C = Wire;
MFRC522DriverI2C driver{ customAddress, customI2C };  // Create I2C driver.
MFRC522 mfrc522{ driver };                            // Create MFRC522 instance.
MFRC522::Uid storedUID;
#endif

#ifdef dht11PIN
xht11 xht(dht11PIN);
#endif


// WiFi
unsigned char dht[4] = { 0, 0, 0, 0 };  //Only the first 32 bits of data are received, not the parity bits
WiFiServer server(80);
WiFiClient esp32Client;
PubSubClient client(esp32Client);

// config
bool config = false;
String mqttNamespaceString;

// other stuff
char msg[50];
int value = 0;

String message = "0";

int neighborhood;
int home;

char bruh[50];

void setup(){
  // Begin Serial
  Serial.begin(115200);

  // Begin Preferences
  preferences.begin("smarthome", false); 

  // Set up LCD
  mylcd.init();
  mylcd.backlight();

  setup_wifi();

  // Set pins
#ifdef LEDPin
  pinMode(LEDPin, OUTPUT);
#endif
#ifdef buzzerPin
  pinMode(buzzerPin,OUTPUT);
#endif
#ifdef motionPin
  pinMode(motionPin, INPUT_PULLUP);
#endif
#ifdef gasPin
  pinMode(gasPin, INPUT_PULLUP);
#endif
#ifdef pushbutton1Pin
  pinMode(pushbutton1Pin, INPUT_PULLUP);
#endif
#ifdef pushbutton2Pin
  pinMode(pushbutton2Pin, INPUT_PULLUP);
#endif
#ifdef touchPin
  pinMode(touchPin, INPUT);
#endif
#ifdef relayPin
  pinMode(relayPin, OUTPUT);
#endif
#ifdef fanPin1
  pinMode(fanPin1, OUTPUT);
#endif
#ifdef fanpin2
  pinMode(fanPin2, OUTPUT);
#endif

  // RFID
#ifdef RFID
  mfrc522.PCD_Init();                                      // Init MFRC522 board.
  MFRC522Debug::PCD_DumpVersionToSerial(mfrc522, Serial);  // Show details of PCD - MFRC522 Card Reader details.
  Serial.println(F("Scan PICC to see UID, SAK, type, and data blocks..."));
#endif 

  //ledcSetup(5, 1200, 8);      //Set the LEDC channel 1 frequency to 1200 and the PWM resolution to 8, that is, the duty cycle is 256.
  //ledcAttachPin(fanPin2, 5);  //Bind LEDC channel 1 to the specified left motor pin gpio26 for output.

  homeNumber = preferences.getInt("home", 0);
  reset = preferences.getBool("reset", false);
  neighborhood  = preferences.getInt("neighborhood", 0);
  home = preferences.getInt("home", 0);
  char mqttnamespace[50];

#ifdef SMARTHOME
  sprintf(mqttnamespace, "Smart Company/Neighborhood %d/Smart Homes/home%d", neighborhood, home);
#endif
#ifdef SMARTFARM
  sprintf(mqttnamespace, "Smart Company/Neighborhood %d/Smart Farms/farm%d", neighborhood, home);
#endif

  sprintf(bruh, "N%d/home%d", neighborhood, home);

  sprintf(client_subscribe_all, "%s/#", mqttnamespace);
  sprintf(control1, "%s/control1", mqttnamespace);
  sprintf(control2, "%s/control2", mqttnamespace);
  sprintf(control3, "%s/control3", mqttnamespace);
  sprintf(control4, "%s/control4", mqttnamespace);
  sprintf(control5, "%s/control5", mqttnamespace);

  sprintf(LEDcolorStrip, "%s/LEDcolorStrip", mqttnamespace);
  sprintf(client_motion, "%s/motion", mqttnamespace);
  sprintf(client_gas, "%s/gas", mqttnamespace);
  sprintf(client_pushbutton1, "%s/pushbutton1", mqttnamespace);
  sprintf(client_pushbutton2, "%s/pushbutton2", mqttnamespace);
  sprintf(client_touch, "%s/touch", mqttnamespace);
  sprintf(client_temperature, "%s/temperature", mqttnamespace);
  sprintf(client_humidity, "%s/humidity", mqttnamespace);
  sprintf(client_water, "%s/water level", mqttnamespace);
  sprintf(client_soil, "%s/soil", mqttnamespace);
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
    client.publish(control5, (char*)message.c_str());
    Serial.println("intital values sent");
  }

  delay(500);
  mylcd.clear();
  mylcd.setCursor(0, 0);
  mylcd.print(bruh);

#ifdef pushbutton1Pin
#ifdef pushbutton2Pin
  if (!reset || (!digitalRead(pushbutton1Pin) && !digitalRead(pushbutton2Pin))){
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Connect to URL:");
    mylcd.setCursor(2, 1);
    mylcd.println(WiFi.localIP().toString());
    server.begin();
    config = true;
  }
#else
  if (!reset || (!digitalRead(pushbutton1Pin))){
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Connect to URL:");
    mylcd.setCursor(2, 1);
    mylcd.println(WiFi.localIP().toString());
    server.begin();
    config = true;
  }
#endif
#endif

}

void loop(){
  static long lastMsg = 0;
  if (config){
    webserver();
    return;
  }
  if (!client.connected()) {
    reconnect();
    delay(500);
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print(mqttNamespaceString.substring(mqttNamespaceString.indexOf("Neighborhood ") + 13).c_str());
  }
  client.loop();

  long now = millis();
  if (now - lastMsg > 1000) {  //update every second
    lastMsg = now;
    count();                 //increase count every second
#ifdef dht11PIN
    temperature_humidity();  //temprature & humidity
#endif
#ifdef touchPin
    touch();
#endif
  }

#ifdef gasPin
  gas();
#endif

#ifdef motionPin
  motion();
#endif

#ifdef pushbutton1Pin
  pushbuttons();
#endif

#ifdef RFID
  rfid();
#endif

#ifdef waterLevelPin
  waterLevel();
#endif

#ifdef soilHumidityPin
  soilHumidity();
#endif
}

// Starts webserver
void webserver(){
  static String readString;
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
          client.println("Enter Neighborhood Number: <INPUT TYPE=number NAME='neighborhood' VALUE=0 min=0>");

          client.println("Enter Home Number: <INPUT TYPE=number NAME='home' VALUE=0 min=0>");
          client.println("<INPUT TYPE=SUBMIT NAME='reset' VALUE='Finalize and Reset'>");
          client.println("</FORM>");


          client.println("<FORM ACTION='/' method=get align='center'>"); //uses IP/port of web page
          client.println("<INPUT TYPE=SUBMIT NAME='default' VALUE='Reset to Default Settings'>");
          client.println("</FORM>");

          client.println("</BODY>");
          client.println("</HTML>");

          delay(1);
          client.stop();

          Serial.println(readString);
          if(readString.indexOf("default=Reset+to+Default+Settings") >0)//checks for off
          {
            preferences.putBool("reset", false);
            reset = false;
          }
          if(readString.indexOf("reset=Finalize+and+Reset") >0)//checks for reset
          {
            String thing = readString.substring(readString.indexOf("?neighborhood=") + 14);
            String neighborhood = thing.substring(0, thing.indexOf("&"));
            thing = thing.substring(thing.indexOf("&home=") + 6);
            String home = thing.substring(0, thing.indexOf("&"));
            preferences.putBool("reset", true);
            preferences.putInt("neighborhood", neighborhood.toInt());
            preferences.putInt("home", home.toInt());
            reset = true;
            abort();
          }
          //clearing string for next read
          readString="";
        }
      }
    }
  }
}

void setup_wifi() {
  Serial.println("hello");
  delay(10);
  mylcd.clear();
  mylcd.setCursor(0, 0);
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
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  delay(1000);
}

//This function reads subscritptions
void callback(char* topic, byte* message, unsigned int length) {
  Serial.println("Message arrived!!!");

  // Serial.print(topic);
  // Serial.print(": ");
  String messageTemp;
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  if (messageTemp == "true" || messageTemp == "True" || messageTemp == "1") {
    messageTemp = "1";
  } else if (messageTemp == "false" || messageTemp == "False" || messageTemp == "0") {
    messageTemp = "0";
  }

  // Motor
  static Servo Dservo;
  static Servo Wservo;

  // Buzzer 
  Buzzer buzz(buzzerPin);

#ifdef SMARTHOME
  if (String(topic) == control1) {
    digitalWrite(LEDPin, messageTemp.toInt());
  }
  if (String(topic) == control2) {

    Dservo.attach(doorServo);
    if (messageTemp.toInt() == 1) Dservo.write(180);  //Open Door
    else Dservo.write(0);                             //Close Door
    delay(500);
  }
  if (String(topic) == control3) {
    Wservo.attach(windowServo);
    if (messageTemp.toInt() == 1) Wservo.write(165);  //Open Window
    else Wservo.write(50);                            //Close Window
    delay(500);
  }
  if (String(topic) == control4 && (messageTemp == "1")) {
    //pinMode(buzzerPin, OUTPUT);
    //pinMode(buzzerPin, INPUT_PULLUP);  //must turn off because of interferance with buzzer
    buzz.sound(165, 100);
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
  if (String(topic) == control5) {
    if (messageTemp.toFloat())
      analogWrite(fanPin1, (messageTemp.toFloat()) * 130 + 125);
    else
      analogWrite(fanPin1, 0);
    digitalWrite(fanPin2, LOW);
  }
#endif
#ifdef SMARTFARM
  if (String(topic) == control1) {
    digitalWrite(LEDPin, messageTemp.toInt());
  }
  if (String(topic) == control2) {
    Dservo.attach(doorServo);
    if (messageTemp.toInt() == 1) Dservo.write(180);  //Open Door
    else Dservo.write(80);                             //Close Door
    delay(500);
  }
  if (String(topic) == control3) {
    buzz.sound(165, 100);
  }
  if (String(topic) == control4) {
    digitalWrite(relayPin, messageTemp.toInt());
  }
  if (String(topic) == control5) {
    if (messageTemp.toFloat())
      analogWrite(fanPin1, (messageTemp.toFloat()) * 130 + 125);
    else
      analogWrite(fanPin1, 0);
    digitalWrite(fanPin2, LOW);
  }
#endif
}

void reconnect() {
  while (!client.connected()) {                     // Loop until reconnected
    Serial.print("Attempting MQTT connection...");  // Attempt to connect
    mylcd.clear();
    mylcd.setCursor(0, 0);
    mylcd.print("Attempting MQTT");
    mylcd.setCursor(0, 1);
    mylcd.print("connection...");
    if (client.connect(("home" + String(homeNumber)).c_str())) {  //Client Name must be unique for every device in the network
      Serial.println("connected");
      // Subscribe
      client.publish(client_message, "Reconnected!");
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
      delay(10000);
    }
  }
}

#ifdef LEDStripPin
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
#endif

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

#ifdef dht11PIN
void temperature_humidity() {

  static float humidity = 0;
  static float oldHumidity = -1;
  static float temperature = 0;
  static float oldTemperature = -1;

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
#endif

#ifdef motionPin
void motion() {
  static bool motionOld = 1;
  static bool motionNew = 0;
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
#endif

#ifdef gasPin
void gas() {
  static bool gasOld = 0;
  static bool gasNew = 1;
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
#endif

#ifdef pushbutton1Pin
void pushbuttons() {
  static bool pushbutton1Old = 0;
  static bool pushbutton1New = 1;

  pushbutton1New = digitalRead(pushbutton1Pin);
  if (pushbutton1Old != pushbutton1New) {
    message = String(pushbutton1New);
    client.publish(client_pushbutton1, (char*)message.c_str());
    Serial.print(client_pushbutton1);
    Serial.print(": ");
    Serial.println(message);
    pushbutton1Old = pushbutton1New;
  }

#ifdef pushbutton2Pin
  static bool pushbutton2Old = 1;
  static bool pushbutton2New = 1;
  pushbutton2New = digitalRead(pushbutton2Pin);
  if (pushbutton2Old != pushbutton2New) {
    message = String(pushbutton2New);
    client.publish(client_pushbutton2, (char*)message.c_str());
    Serial.print(client_pushbutton2);
    Serial.print(": ");
    Serial.println(message);
    pushbutton2Old = pushbutton2New;
  }
#endif
}
#endif

#ifdef touchPin
void touch() {
  static int touchOld = -1;
  static int touchNew = 0;
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
#endif

#ifdef RFID
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
#endif

#ifdef waterLevelPin
void waterLevel(){
  static int waterLevelOld = -1;
  static int waterLevelNew = 0;
  waterLevelNew = analogRead(waterLevelPin);
  if (waterLevelNew != waterLevelOld) {
    message = String(waterLevelNew);
    client.publish(client_water, (char*)message.c_str());
    Serial.print(client_touch);
    Serial.print(": ");
    Serial.println(message);
    waterLevelOld = waterLevelNew;
  }
}
#endif
#ifdef soilHumidityPin
void soilHumidity(){
  static int waterLevelOld = -1;
  static int waterLevelNew = 0;
  waterLevelNew = analogRead(soilHumidityPin);
  if (waterLevelNew != waterLevelOld) {
    message = String(waterLevelNew);
    client.publish(client_soil, (char*)message.c_str());
    Serial.print(client_touch);
    Serial.print(": ");
    Serial.println(message);
    waterLevelOld = waterLevelNew;
  }
}
#endif
