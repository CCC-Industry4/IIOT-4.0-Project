#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>     
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <DNSServer.h> 

#include <Wire.h>
#include "xht11.h"          
#include <SPI.h>

// --- KEYESTUDIO OFFICIAL LIBRARY INCLUDES ---
#include <MFRC522_I2C.h> 

#include <Adafruit_NeoPixel.h>
#include <Buzzer.h>          
#include <ESP32Servo.h>

#include <LiquidCrystal_I2C.h>  

// WIP COMM HERE
int value = 0;
int homeNumber = 1;
const char* ssid = "IT4Project";  
const char* password = "IOT12345";
const char* mqtt_server = "192.168.10.2";

#define SMARTHOME

char ssid_config[32];
char pass_config[32];
char mqtt_server_config[40];  

const char* wifi_ssid = nullptr;
const char* wifi_pass = nullptr;

#include "pins.h"
#define MAX_RULES 20 
#define MAX_FOBS 5 // Support up to 5 distinct Key Fobs

struct Rule {
  String source;    
  String op;        
  String value;     
  String action[3]; 
  String extra[3];  
};

Rule smartHomeRules[MAX_RULES];
bool ruleLastState[MAX_RULES] = {false};

String knownFobs[MAX_FOBS];

Servo Dservo;
Servo Wservo;

bool fanIsOn = false;
bool ledIsOn = false;
bool doorIsOpen = false;
bool windowIsOpen = false;
bool stripIsOn = false;

bool offlineMode = false;
bool isConfigMode = false; 
String apNameStr = "";
String apPassStr = "";

int totalButtonPresses = 0;
float lastTemp = 0.0;
float lastHum = 0.0;

int lcdTop = 0;
int lcdBL = 1;
int lcdBR = 2;

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
static char client_rfid[100];
static char client_message[100];
static char client_subscribe_all[100];
static char client_motion[100];
static char client_pushbutton1[100];
static char client_pushbutton2[100];
static char client_gas[100];
static char client_touch[100];
static char client_water[100];
static char client_soil[100];

Preferences preferences; 
DNSServer dnsServer; 

#ifdef LEDStripPin
Adafruit_NeoPixel strip(LED_COUNT, LEDStripPin, NEO_GRB + NEO_KHZ800);
#endif

LiquidCrystal_I2C mylcd(0x27, 16, 2);

#ifdef RFID
MFRC522 mfrc522(0x28); 
MFRC522::Uid storedUID;
#endif

#ifdef dht11PIN
xht11 xht(dht11PIN);
#endif

unsigned char dht[4] = { 0, 0, 0, 0 };  
WebServer server(80); 
WiFiClient esp32Client;
PubSubClient client(esp32Client);

bool reset = false;
int neighborhood;
int home;
char bruh[50];

void updateLcdWebserver();
void setupWebHandlers();
bool connectWiFi(int timeoutSec);
void handleWiFiFailure();
void reconnect();
void count();
void updateLCD();
void onClientConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void callback(char* topic, byte* message, unsigned int length);
void executeAllActions(Rule r);
void executeAction(String action, String extra);
void stripOff();
void evaluateRules(String event = "");

#ifdef LEDStripPin
void colorWipe(uint32_t color, int wait);
void rainbow(int wait);
#endif

#ifdef dht11PIN
void temperature_humidity();
#endif
#ifdef motionPin
void motion();
#endif
#ifdef gasPin
void gas();
#endif
#ifdef touchPin
void touch();
#endif
#ifdef RFID
String rfid();
#endif
#ifdef waterLevelPin
void waterLevel();
#endif
#ifdef soilHumidityPin
void soilHumidity();
#endif

struct ButtonHandler {
  int pin;
  String name;
  bool lastState = HIGH;
  unsigned long pressTime = 0;
  unsigned long releaseTime = 0;
  int clicks = 0;
  bool timing = false;
  bool held = false;

  ButtonHandler(int p, String n) : pin(p), name(n) {}

  String update() {
    if (pin < 0) return "";
    bool currentState = digitalRead(pin);
    unsigned long now = millis();
    String event = "";

    if (currentState == LOW && lastState == HIGH) {
      pressTime = now;
      held = false;
    }
    else if (currentState == HIGH && lastState == LOW) {
      releaseTime = now;
      if (!held) {
        clicks++;
        timing = true;
      }
    }

    if (currentState == LOW && !held && (now - pressTime) > 600) {
      held = true;
      timing = false;
      clicks = 0;
      event = "hold";
    }

    if (currentState == HIGH && timing && (now - releaseTime) > 250) {
      if (clicks == 1) event = "tap";
      else if (clicks >= 2) event = "double";
      clicks = 0;
      timing = false;
    }

    lastState = currentState;
    return event;
  }
};

#ifdef pushbutton1Pin
ButtonHandler btn1(pushbutton1Pin, "btn1");
#else
ButtonHandler btn1(-1, "btn1");
#endif

#ifdef pushbutton2Pin
ButtonHandler btn2(pushbutton2Pin, "btn2");
#else
ButtonHandler btn2(-1, "btn2");
#endif

void setup() {
  Serial.begin(115200);
  preferences.begin("smarthome", false);
  
  for(int i=0; i<MAX_FOBS; i++) knownFobs[i] = preferences.getString(("fob"+String(i)).c_str(), "");

  for (int i = 0; i < MAX_RULES; i++) {
    smartHomeRules[i].source = preferences.getString(("src" + String(i)).c_str(), "none");
    smartHomeRules[i].op     = preferences.getString(("op" + String(i)).c_str(), "==");
    smartHomeRules[i].value  = preferences.getString(("val" + String(i)).c_str(), "");
    for(int a=0; a<3; a++) {
      smartHomeRules[i].action[a] = preferences.getString(("a" + String(i) + "_" + String(a)).c_str(), "none");
      smartHomeRules[i].extra[a]  = preferences.getString(("ext" + String(i) + "_" + String(a)).c_str(), "");
    }
  }

  lcdTop = preferences.getInt("lt", 0);
  lcdBL = preferences.getInt("lbl", 1);
  lcdBR = preferences.getInt("lbr", 2);

#ifdef doorServo
  Dservo.attach(doorServo);
  Dservo.write(0); 
#endif
#ifdef windowServo
  Wservo.attach(windowServo);
  Wservo.write(50); 
#endif

  Wire.begin(); 
  mylcd.init();
  mylcd.backlight();

#ifdef RFID
  mfrc522.PCD_Init();
#endif

#ifdef LEDStripPin
  strip.begin(); 
  strip.show(); 
#endif

#ifdef LEDPin
  pinMode(LEDPin, OUTPUT);
#endif
#ifdef buzzerPin
  pinMode(buzzerPin, OUTPUT);
#endif
#ifdef motionPin
  pinMode(motionPin, INPUT); 
#endif
#ifdef gasPin
  pinMode(gasPin, INPUT); 
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
#ifdef fanPin1
  pinMode(fanPin1, OUTPUT);
#endif
#ifdef fanPin2
  pinMode(fanPin2, OUTPUT);
#endif

  reset = preferences.getBool("reset", false);
  delay(500); 

  mylcd.clear();
  mylcd.setCursor(0, 0); mylcd.print("Press both btns");
  mylcd.setCursor(0, 1); mylcd.print("for Config (3s)");
  
  unsigned long bootTime = millis();
  while (millis() - bootTime < 3000) {
#if defined(pushbutton1Pin) and defined(pushbutton2Pin)
    if (digitalRead(pushbutton1Pin) == LOW && digitalRead(pushbutton2Pin) == LOW) {
      isConfigMode = true;
      mylcd.clear();
      mylcd.setCursor(0, 0); mylcd.print("Config Mode");
      mylcd.setCursor(0, 1); mylcd.print("Activated!");
      delay(1000);
      break;
    }
#endif
    delay(50);
  }

  if (!reset || isConfigMode) {
    uint16_t entropyName = esp_random() & 0xFFFF;
    uint32_t entropyPass = esp_random(); 

    apNameStr = "CONFIG_" + String(entropyName, HEX);
    apNameStr.toUpperCase();
    apPassStr = String(entropyPass, HEX);
    apPassStr.toUpperCase();
    while(apPassStr.length() < 8) apPassStr += "0";

    WiFi.mode(WIFI_AP);
    IPAddress apIP(192, 168, 4, 1);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    
    // FIX 3: Moved to Wi-Fi Channel 6 (less congested)
    WiFi.softAP(apNameStr.c_str(), apPassStr.c_str(), 6);
    
    // FIX 1 (Revised): Use explicit ESP32 flag for no power saving
    WiFi.setSleep(WIFI_PS_NONE);
    delay(100); 
    
    dnsServer.start(53, "*", apIP);
    setupWebHandlers();
    server.begin();
    return; 
  }

  String storedSSID  = preferences.getString("wifi_ssid", ssid);
  String storedPASS  = preferences.getString("wifi_pass", password);
  storedSSID.toCharArray(ssid_config, 32);
  storedPASS.toCharArray(pass_config, 32);

  if (!reset) {
    wifi_ssid = ssid;
    wifi_pass = password;
  } else {
    wifi_ssid = ssid_config;
    wifi_pass = pass_config;
  }
  
  mylcd.clear();
  mylcd.setCursor(0, 0); mylcd.print("Connecting WiFi");
  
  if (!connectWiFi(10)) handleWiFiFailure();
  else WiFi.setSleep(WIFI_PS_NONE);

  homeNumber = preferences.getInt("home", 1);
  neighborhood = preferences.getInt("neighborhood", 1);
  home = preferences.getInt("home", 1);
  
  if (!offlineMode) {
    String storedMQTT = preferences.getString("mqtt_ip", mqtt_server);
    storedMQTT.toCharArray(mqtt_server_config, 40);
    client.setServer(mqtt_server_config, 1883);
    client.setCallback(callback);
  }

  char mqttnamespace[100]; 
#ifdef SMARTHOME
  sprintf(mqttnamespace, "Smart Company/Neighborhood %d/Smart Homes/home%d", neighborhood, home);
#endif
  sprintf(bruh, "N%d/H%d", neighborhood, home);
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

  if (!offlineMode) {
    reconnect();
    delay(500);
    client.publish(LEDcolorStrip, "0");
  }
  delay(500);
  updateLCD();
}

void loop() {
  static long lastMsg = 0;
  
  if (isConfigMode || !reset) {
    dnsServer.processNextRequest(); 
    updateLcdWebserver();
    server.handleClient(); 
    delay(2); 
    return;
  }
  
  if (!offlineMode) {
    if (!client.connected()) {
      static unsigned long lastMqttRetry = 0;
      if (millis() - lastMqttRetry > 5000) {
        lastMqttRetry = millis();
        reconnect();
        if (client.connected()) updateLCD();
      }
    } else {
      client.loop(); 
    }
  }

  long now = millis();
  if (now - lastMsg > 1000) { 
    lastMsg = now;
    count();  
#ifdef dht11PIN
    temperature_humidity();  
#endif
#ifdef touchPin
    touch();
#endif
    updateLCD(); 
    evaluateRules(""); 
  }

#ifdef gasPin
  gas();
#endif
#ifdef motionPin
  motion();
#endif
#ifdef waterLevelPin
  waterLevel();
#endif
#ifdef soilHumidityPin
  soilHumidity();
#endif
#ifdef RFID
  String currentRfid = rfid();
  if (currentRfid != "") {
    int matchedFob = -1;
    for(int i=0; i<MAX_FOBS; i++) {
       if (knownFobs[i] == currentRfid) { matchedFob = i + 1; break; }
    }
    if (matchedFob != -1) evaluateRules("fob" + String(matchedFob));
  }
#endif

  String e1 = btn1.update();
  if (e1 != "") {
    totalButtonPresses++;
    if (!offlineMode) client.publish(client_pushbutton1, (char*)("btn1_"+e1).c_str());
    evaluateRules("btn1_" + e1);
  }

  String e2 = btn2.update();
  if (e2 != "") {
    totalButtonPresses++;
    if (!offlineMode) client.publish(client_pushbutton2, (char*)("btn2_"+e2).c_str());
    evaluateRules("btn2_" + e2);
  }
  delay(10); 
}

void evaluateRules(String triggerEvent) {
  for (int i = 0; i < MAX_RULES; i++) {
    Rule r = smartHomeRules[i];
    if (r.source == "none") continue;
    bool conditionMet = false;
    bool isContinuousBtn = ((r.source == "btn1" || r.source == "btn2") && r.value == "raw");

    if (isContinuousBtn) {
#ifdef pushbutton1Pin
      if (r.source == "btn1") conditionMet = (digitalRead(pushbutton1Pin) == LOW);
#endif
#ifdef pushbutton2Pin
      if (r.source == "btn2") conditionMet = (digitalRead(pushbutton2Pin) == LOW);
#endif
    } 
    else if (r.source.startsWith("btn") || r.source.startsWith("fob")) {
      if (r.source == "fob_any") {
         if (triggerEvent.startsWith("fob")) conditionMet = true;
      } 
      else if (r.source.startsWith("fob")) {
         if (triggerEvent == r.source) conditionMet = true;
      }
      else if (r.source.startsWith("btn")) {
         if (triggerEvent == r.source + "_" + r.value) conditionMet = true;
      }
    } 
    else {
      float currentVal = 0.0;
      if (r.source == "temp") currentVal = lastTemp;
      else if (r.source == "hum") currentVal = lastHum;
      else if (r.source == "motion") {
#ifdef motionPin
        currentVal = digitalRead(motionPin); 
#endif
      }
      else if (r.source == "touch") {
#ifdef touchPin
        currentVal = touchRead(touchPin);
#endif
      }
      else if (r.source == "gas") {
#ifdef gasPin
        currentVal = digitalRead(gasPin);
#endif
      }
      else if (r.source == "water") {
#ifdef waterLevelPin
        currentVal = analogRead(waterLevelPin);
#endif
      }
      else if (r.source == "soil") {
#ifdef soilHumidityPin
        currentVal = analogRead(soilHumidityPin);
#endif
      }

      float targetVal = r.value.toFloat();
      if (r.op == ">") conditionMet = (currentVal > targetVal);
      else if (r.op == "<") conditionMet = (currentVal < targetVal);
      else if (r.op == "==") conditionMet = (currentVal == targetVal);
    }

    if (conditionMet) {
      if (!isContinuousBtn && (r.source.startsWith("btn") || r.source.startsWith("fob"))) {
        executeAllActions(r);
      } 
      else {
        bool isToggleOrFlash = false;
        for(int a=0; a<3; a++) {
            if (r.action[a].indexOf("toggle") >= 0 || r.action[a] == "strip_flash") isToggleOrFlash = true;
        }
        if (isToggleOrFlash) {
          if (!ruleLastState[i]) executeAllActions(r); 
        } 
        else {
          executeAllActions(r);
        }
      }
    } 
    else if (ruleLastState[i] && (isContinuousBtn || (!r.source.startsWith("btn") && !r.source.startsWith("fob")))) {
       for(int a=0; a<3; a++) {
           if (r.action[a] == "strip_momentary") stripOff();
       }
    }

    if (isContinuousBtn || (!r.source.startsWith("btn") && !r.source.startsWith("fob"))) {
      ruleLastState[i] = conditionMet;
    }
  }
}

void applyCustomColors(String extra) {
#ifdef LEDStripPin
  int colors[12] = {0};
  int startIdx = 0;
  for(int i=0; i<12; i++) {
      int commaIdx = extra.indexOf(',', startIdx);
      if (commaIdx == -1) commaIdx = extra.length();
      colors[i] = extra.substring(startIdx, commaIdx).toInt();
      startIdx = commaIdx + 1;
  }
  for (int i = 0; i < 4; i++) {
    if (i < strip.numPixels()) strip.setPixelColor(i, strip.Color(colors[i*3], colors[i*3+1], colors[i*3+2]));
  }
  strip.show();
#endif
}

void stripOff() {
#ifdef LEDStripPin
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 0));
  strip.show();
#endif
}

void executeAllActions(Rule r) {
    for(int a=0; a<3; a++) {
        if(r.action[a] != "none" && r.action[a] != "") executeAction(r.action[a], r.extra[a]);
    }
}

void executeAction(String action, String extra) {
  Buzzer buzz(buzzerPin);
  if (action == "fan_toggle") { fanIsOn = !fanIsOn;
#ifdef fanPin1
    analogWrite(fanPin1, fanIsOn ? 255 : 0); digitalWrite(fanPin2, LOW);
#endif
  } 
  else if (action == "fan_on") { fanIsOn = true;
#ifdef fanPin1
    analogWrite(fanPin1, 255); digitalWrite(fanPin2, LOW);
#endif
  }
  else if (action == "fan_off") { fanIsOn = false;
#ifdef fanPin1
    analogWrite(fanPin1, 0); digitalWrite(fanPin2, LOW);
#endif
  }
  else if (action == "led_toggle") { ledIsOn = !ledIsOn;
#ifdef LEDPin
    digitalWrite(LEDPin, ledIsOn ? HIGH : LOW);
#endif
  }
  else if (action == "door_toggle") { doorIsOpen = !doorIsOpen;
#ifdef doorServo
    Dservo.write(doorIsOpen ? 180 : 0);
#endif
  }
  else if (action == "door_open") { doorIsOpen = true;
#ifdef doorServo
    Dservo.write(180);
#endif
  }
  else if (action == "door_close") { doorIsOpen = false;
#ifdef doorServo
    Dservo.write(0);
#endif
  }
  else if (action == "window_toggle") { windowIsOpen = !windowIsOpen;
#ifdef windowServo
    Wservo.write(windowIsOpen ? 165 : 50);
#endif
  }
  else if (action == "window_open") { windowIsOpen = true;
#ifdef windowServo
    Wservo.write(165);
#endif
  }
  else if (action == "window_close") { windowIsOpen = false;
#ifdef windowServo
    Wservo.write(50);
#endif
  }
  else if (action == "buzzer_beep") buzz.sound(165, 200);
  else if (action == "strip_toggle") {
    stripIsOn = !stripIsOn;
    if (stripIsOn) applyCustomColors(extra); else stripOff();
  }
  else if (action == "strip_momentary") { stripIsOn = true; applyCustomColors(extra); }
  else if (action == "strip_flash") { applyCustomColors(extra); delay(100); stripOff(); }
}

void updateLCD() {
  auto getDisplayString = [](int opt, int length) {
    String s = "";
    if (opt == 0) {
      if (offlineMode) s = "Offline "; else s = String(bruh) + " ";
      int d = value / 86400; int h = (value % 86400) / 3600; int m = (value % 3600) / 60; int sec = value % 60;
      char buf[20]; sprintf(buf, "%d:%02d:%02d:%02d", d, h, m, sec);
      s += String(buf);
    }
    else if (opt == 1) s = String(lastTemp, 1) + "C";
    else if (opt == 2) s = String(lastHum, 1) + "%";
    else if (opt == 3) s = String(totalButtonPresses);
    else if (opt == 4) s = String(value);
    
    while (s.length() < length) s += " ";
    return s.substring(0, length);
  };
  mylcd.setCursor(0, 0); mylcd.print(getDisplayString(lcdTop, 16));
  mylcd.setCursor(0, 1); mylcd.print(getDisplayString(lcdBL, 8));
  mylcd.setCursor(8, 1); mylcd.print(getDisplayString(lcdBR, 8));
}

void updateLcdWebserver() {
  static unsigned long lastLcdUpdate = 0;
  if (millis() - lastLcdUpdate > 1000) {
    lastLcdUpdate = millis();
    if (WiFi.softAPgetStationNum() > 0) {
      mylcd.setCursor(0, 0); mylcd.print("Go to IP:       ");
      mylcd.setCursor(0, 1); mylcd.print("192.168.4.1     ");
    } else {
      mylcd.setCursor(0, 0); 
      mylcd.print((apNameStr.length() > 16) ? apNameStr.substring(0,16) : apNameStr);
      while(apNameStr.length() < 16) { mylcd.print(" "); apNameStr += " "; }
      mylcd.setCursor(0, 1); 
      String passStr = "PW:" + apPassStr;
      mylcd.print(passStr);
      while(passStr.length() < 16) { mylcd.print(" "); passStr += " "; }
    }
  }
}

void setupWebHandlers() {
  server.on("/", []() {
    server.sendHeader("Connection", "close"); // FIX 1: Immediately close TCP socket
    
    if (server.hasArg("default")) {
      preferences.putBool("reset", false); 
      reset = false; 
      server.send(200, "text/html", "<html><body><h2>Reset to defaults. Rebooting...</h2></body></html>");
      delay(1000); ESP.restart(); return;
    }

    if (server.hasArg("reset")) {
      int nh = server.arg("neighborhood").toInt(); int hm = server.arg("home").toInt();
      if(nh < 1) nh = 1; if(nh > 100) nh = 100; if(hm < 1) hm = 1; if(hm > 100) hm = 100;
      preferences.putBool("reset", true);
      preferences.putInt("neighborhood", nh); preferences.putInt("home", hm);
      preferences.putString("mqtt_ip", server.arg("mqttip"));
      preferences.putString("wifi_ssid", server.arg("wifissid")); preferences.putString("wifi_pass", server.arg("wifipass"));
      server.send(200, "text/html", "<html><body><h2>Settings Saved. Rebooting...</h2></body></html>");
      delay(1000); ESP.restart(); return;
    }

    int storedNeighborhood = preferences.getInt("neighborhood", 1);
    int storedHome = preferences.getInt("home", 1);
    String storedMQTT = preferences.getString("mqtt_ip", mqtt_server);

    String page = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Config</title><style>body{font-size:18px; font-family:Arial; padding:10px;} input,button{font-size:18px; padding:6px; margin:5px 0;} h2{font-size:24px;}</style></head><body>";
    page += "<h2>Smart Home Network Configuration</h2><form method='get' action='/'>";
    page += "Neighborhood (1-100): <br><input type='number' name='neighborhood' value='" + String(storedNeighborhood) + "' min='1' max='100'><br>";
    page += "Home Number (1-100): <br><input type='number' name='home' value='" + String(storedHome) + "' min='1' max='100'><br>";
    page += "MQTT IP: <br><input type='text' name='mqttip' value='" + String(storedMQTT) + "'><br>";
    page += "WiFi SSID: <br><input type='text' name='wifissid' value='" + String(ssid_config) + "'><br>";
    page += "WiFi Pass: <br><input type='text' name='wifipass' value='" + String(pass_config) + "'><br><br>";
    page += "<input type='submit' name='reset' value='Finalize and Reset ESP32' style='background:#4CAF50; color:white; border:none; padding:10px; cursor:pointer;'></form><br>";
    page += "<form method='get' action='/'><input type='submit' name='default' value='Reset to Defaults'></form><br><hr><br>";
    page += "<a href='/rules'><button type='button'>Go to Sandbox / Rules Config</button></a><br><br>";
    page += "<a href='/fobs'><button type='button' style='background:#2196F3; color:white; border:none; padding:10px;'>Manage Key Fobs</button></a></body></html>";
    server.send(200, "text/html", page);
  });

  server.on("/fobs", []() {
    server.sendHeader("Connection", "close"); 
    String p = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>body{font-family:Arial;font-size:18px; padding:10px;} .box{background:#eee;padding:10px;margin-bottom:10px;border-radius:5px;} button{font-size:16px; padding:6px; cursor:pointer;}</style></head><body><h2>Manage Key Fobs</h2>";
    for(int i=0; i<MAX_FOBS; i++) {
      p += "<div class='box'><b>Fob Slot " + String(i+1) + "</b><br>";
      p += (knownFobs[i] == "") ? "Status: Not Paired<br>" : "UID: " + knownFobs[i] + "<br>";
      p += "<form action='/pairfob' method='get' style='display:inline;'><input type='hidden' name='slot' value='" + String(i) + "'><button type='submit' style='background:#4CAF50; color:white; border:none;'>Pair New</button></form> ";
      p += "<form action='/clearfob' method='get' style='display:inline;'><input type='hidden' name='slot' value='" + String(i) + "'><button type='submit' style='background:#f44336; color:white; border:none;'>Clear Slot</button></form></div>";
    }
    p += "<br><a href='/'><button>Back to Network Config</button></a></body></html>";
    server.send(200, "text/html", p);
  });

  server.on("/clearfob", []() {
    server.sendHeader("Connection", "close");
    int s = server.hasArg("slot") ? server.arg("slot").toInt() : -1;
    if(s >= 0 && s < MAX_FOBS) { knownFobs[s] = ""; preferences.putString(("fob"+String(s)).c_str(), ""); }
    server.sendHeader("Location", "/fobs"); server.send(302, "text/plain", "");
  });

  server.on("/pairfob", []() {
    server.sendHeader("Connection", "close");
    int s = server.hasArg("slot") ? server.arg("slot").toInt() : -1;
    if (s < 0 || s >= MAX_FOBS) { server.sendHeader("Location", "/fobs"); server.send(302, "text/plain", ""); return; }

    mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("TAP FOB NOW");
    mylcd.setCursor(0, 1); mylcd.print("Waiting 10s...");
    String scannedUID = ""; unsigned long startWait = millis();
    while(millis() - startWait < 10000) {
#ifdef RFID
      scannedUID = rfid(); if(scannedUID != "") break;
#endif
      delay(50);
    }
    if(scannedUID != "") {
      mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("SUCCESS!");
      mylcd.setCursor(0, 1); mylcd.print(scannedUID);
      knownFobs[s] = scannedUID; preferences.putString(("fob"+String(s)).c_str(), scannedUID); delay(1500);
    } else { mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("TIMEOUT"); delay(1500); }
    server.sendHeader("Location", "/fobs"); server.send(302, "text/plain", "");
  });

  // FIX 2: Pre-allocate a large string to blast the response instantly, bypassing TCP chunking delays
  server.on("/rules", []() {
    server.sendHeader("Connection", "close"); 
    
    int editId = server.hasArg("id") ? server.arg("id").toInt() : 0;
    if (editId < 0) editId = 0; if (editId >= MAX_RULES) editId = MAX_RULES - 1;

    String page;
    page.reserve(10000); // Reserve memory upfront to avoid fragmentation
    
    page += "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><style>"
            "body{font-size:18px; font-family:Arial; margin:15px;} select,input,button{font-size:16px; padding:6px; margin-bottom:8px;} "
            ".box{background:#f4f4f4; padding:12px; border-radius:8px;}</style>"
            "<script>"
            "function updateUI() {"
            "  var src = document.getElementById('srcSelect').value;"
            "  var opC = document.getElementById('opContainer');"
            "  var vIn = document.getElementById('valInput');"
            "  var vBtn = document.getElementById('valSelBtn');"
            "  var vBool = document.getElementById('valSelBool');"
            "  vIn.style.display = 'none'; vIn.disabled = true;"
            "  vBtn.style.display = 'none'; vBtn.disabled = true;"
            "  vBool.style.display = 'none'; vBool.disabled = true;"
            "  if(src === 'btn1' || src === 'btn2') {"
            "    opC.style.display = 'none'; vBtn.style.display = 'inline-block'; vBtn.disabled = false;"
            "  } else if(src.indexOf('fob') === 0) {"
            "    opC.style.display = 'none';"
            "  } else if(src === 'motion' || src === 'gas') {"
            "    opC.style.display = 'inline-block'; document.getElementById('opSelect').value = '==';"
            "    vBool.style.display = 'inline-block'; vBool.disabled = false;"
            "  } else {"
            "    opC.style.display = 'inline-block'; vIn.style.display = 'inline-block'; vIn.disabled = false; vIn.type = 'number'; vIn.step = 'any';"
            "  }"
            "  for(var i=0; i<3; i++) updateActionUI(i);"
            "}"
            "function updateActionUI(idx) {"
            "  var act = document.getElementById('actSelect_' + idx).value;"
            "  var ledDiv = document.getElementById('ledConfig_' + idx);"
            "  if(act && act.indexOf('strip_') === 0) ledDiv.style.display = 'block';"
            "  else if (ledDiv) ledDiv.style.display = 'none';"
            "}"
            "window.onload = updateUI;"
            "</script></head><body><h2>Rule Sandbox Configuration</h2>";

    page += "<form method='get' action='/rules'>Select Rule to Edit: <select name='id' onchange='this.form.submit()'>";
    int firstEmpty = -1;
    for(int i=0; i<MAX_RULES; i++){
      if(smartHomeRules[i].source != "none") {
        page += "<option value='" + String(i) + "'" + (i == editId ? " selected" : "") + ">Rule " + String(i+1) + "</option>";
      } else if (firstEmpty == -1) firstEmpty = i;
    }
    if (firstEmpty != -1 && smartHomeRules[editId].source == "none") page += "<option value='" + String(editId) + "' selected>+ New Rule (Slot " + String(editId+1) + ")</option>";
    else if (firstEmpty != -1) page += "<option value='" + String(firstEmpty) + "'>+ Add New Rule</option>";
    page += "</select></form><hr>";

    auto getOpt = [](String val, String label, String savedVal) { return "<option value='" + val + "'" + (savedVal == val ? " selected" : "") + ">" + label + "</option>"; };

    page += "<form method='get' action='/saverule' class='box'><input type='hidden' name='id' value='" + String(editId) + "'>";
    page += "<b>Editing Rule " + String(editId+1) + "</b><br><br>IF Source: <select id='srcSelect' name='src' onchange='updateUI()'>";
    page += getOpt("none", "None", smartHomeRules[editId].source) + getOpt("btn1", "Button 1", smartHomeRules[editId].source) + getOpt("btn2", "Button 2", smartHomeRules[editId].source);
    page += getOpt("fob_any", "Any Registered Fob", smartHomeRules[editId].source);
    for(int i=0; i<MAX_FOBS; i++) page += getOpt("fob" + String(i+1), "Fob Slot " + String(i+1), smartHomeRules[editId].source);
    page += getOpt("temp", "Temperature", smartHomeRules[editId].source) + getOpt("hum", "Humidity", smartHomeRules[editId].source) + getOpt("touch", "Touch Sensor", smartHomeRules[editId].source);
    page += getOpt("motion", "Motion Sensor", smartHomeRules[editId].source) + getOpt("gas", "Gas Sensor", smartHomeRules[editId].source) + getOpt("water", "Water Level", smartHomeRules[editId].source) + getOpt("soil", "Soil Humidity", smartHomeRules[editId].source);
    page += "</select><br>";

    page += "<span id='opContainer'>Operator: <select id='opSelect' name='op'>" + getOpt("==", "Is (==)", smartHomeRules[editId].op) + getOpt(">", "Greater (>)", smartHomeRules[editId].op) + getOpt("<", "Less (<)", smartHomeRules[editId].op) + "</select><br></span>";

    String curVal = smartHomeRules[editId].value;
    page += "Value: <input type='text' id='valInput' name='val' value='" + curVal + "' placeholder='...'>";
    page += "<select id='valSelBtn' name='val' style='display:none;'>" + getOpt("tap", "Single Tap", curVal) + getOpt("double", "Double Tap", curVal) + getOpt("hold", "Hold", curVal) + getOpt("raw", "Direct Press (While Holding)", curVal) + "</select>";
    page += "<select id='valSelBool' name='val' style='display:none;'>" + getOpt("1", "True / Detected (1)", curVal) + getOpt("0", "False / Clear (0)", curVal) + "</select><br><br><hr style='border:1px solid #ddd;'>";

    for(int a=0; a<3; a++) {
        page += "<b>THEN Action " + String(a+1) + ":</b> <select id='actSelect_" + String(a) + "' name='a" + String(a) + "' onchange='updateActionUI(" + String(a) + ")'>";
        page += getOpt("none", "None", smartHomeRules[editId].action[a]) + getOpt("fan_toggle", "Toggle Fan", smartHomeRules[editId].action[a]) + getOpt("fan_on", "Turn Fan ON", smartHomeRules[editId].action[a]) + getOpt("fan_off", "Turn Fan OFF", smartHomeRules[editId].action[a]);
        page += getOpt("led_toggle", "Toggle LED", smartHomeRules[editId].action[a]) + getOpt("door_toggle", "Toggle Door", smartHomeRules[editId].action[a]) + getOpt("door_open", "Open Door", smartHomeRules[editId].action[a]) + getOpt("door_close", "Close Door", smartHomeRules[editId].action[a]);
        page += getOpt("window_toggle", "Toggle Window", smartHomeRules[editId].action[a]) + getOpt("window_open", "Open Window", smartHomeRules[editId].action[a]) + getOpt("window_close", "Close Window", smartHomeRules[editId].action[a]) + getOpt("buzzer_beep", "Beep Buzzer", smartHomeRules[editId].action[a]);
        page += getOpt("strip_toggle", "Toggle Custom LED Strip", smartHomeRules[editId].action[a]) + getOpt("strip_momentary", "Custom LED Strip (Momentary)", smartHomeRules[editId].action[a]) + getOpt("strip_flash", "Flash Custom LED Strip", smartHomeRules[editId].action[a]) + "</select><br>";

        page += "<div id='ledConfig_" + String(a) + "' style='display:none; margin-top:5px; margin-bottom:15px; padding:10px; background:#ddd; border-radius:5px;'><b>LED Strip Colors (0-255)</b><br>";
        int c[12] = {0}; String curExt = smartHomeRules[editId].extra[a];
        if (curExt.length() > 0) {
            int startIdx = 0;
            for(int i=0; i<12; i++) { int commaIdx = curExt.indexOf(',', startIdx); if (commaIdx == -1) commaIdx = curExt.length(); c[i] = curExt.substring(startIdx, commaIdx).toInt(); startIdx = commaIdx + 1; }
        }
        for(int i=1; i<=4; i++) {
           page += "LED " + String(i) + ": R <input type='number' name='r" + String(i) + "_" + String(a) + "' value='" + String(c[(i-1)*3]) + "' min='0' max='255' style='width:60px;'> G <input type='number' name='g" + String(i) + "_" + String(a) + "' value='" + String(c[(i-1)*3+1]) + "' min='0' max='255' style='width:60px;'> B <input type='number' name='b" + String(i) + "_" + String(a) + "' value='" + String(c[(i-1)*3+2]) + "' min='0' max='255' style='width:60px;'><br>";
        }
        page += "</div><br>";
    }

    page += "<button type='submit' name='act' value='save' style='background:#4CAF50; color:white; border:none; padding:8px 16px; border-radius:4px;'>Save Rule</button> <button type='submit' name='act' value='del' style='background:#f44336; color:white; border:none; padding:8px 16px; border-radius:4px;'>Delete Rule</button></form><br><form method='get' action='/'><input type='submit' value='Back to Network Config'></form></body></html>";
    
    server.send(200, "text/html", page);
  });

  server.on("/saverule", []() {
    server.sendHeader("Connection", "close");
    int idx = server.hasArg("id") ? server.arg("id").toInt() : -1;
    String act_param = server.arg("act");
    
    if (idx >= 0 && idx < MAX_RULES) {
      if (act_param == "del") {
        smartHomeRules[idx].source = "none"; smartHomeRules[idx].op = "=="; smartHomeRules[idx].value = "";
        for(int a=0; a<3; a++) { smartHomeRules[idx].action[a] = "none"; smartHomeRules[idx].extra[a] = ""; }
      } else {
        String s = server.arg("src"); String o = server.arg("op"); String v = server.arg("val");
        if (s == "btn1" || s == "btn2" || s.startsWith("fob")) o = "=="; 
        else if (s == "motion" || s == "gas") { o = "=="; if (v != "0" && v != "1") v = "1"; } 
        else if (s != "none") { float checkNum = v.toFloat(); if (checkNum == 0.0 && v != "0" && v != "0.0") v = "0"; }
        smartHomeRules[idx].source = s; smartHomeRules[idx].op = o; smartHomeRules[idx].value = v;

        for(int a=0; a<3; a++) {
            String actStr = server.arg("a" + String(a));
            if (actStr.indexOf("strip_") == 0) {
              String ext = "";
              auto clamp = [](String val) { if (val == "") return 0; int v = val.toInt(); return (v < 0) ? 0 : ((v > 255) ? 255 : v); };
              for (int j = 1; j <= 4; j++) ext += String(clamp(server.arg("r" + String(j) + "_" + String(a)))) + "," + String(clamp(server.arg("g" + String(j) + "_" + String(a)))) + "," + String(clamp(server.arg("b" + String(j) + "_" + String(a)))) + (j == 4 ? "" : ",");
              smartHomeRules[idx].extra[a] = ext;
            } else smartHomeRules[idx].extra[a] = "";
            smartHomeRules[idx].action[a] = actStr;
        }
      }
      preferences.putString(("src" + String(idx)).c_str(), smartHomeRules[idx].source);
      preferences.putString(("op" + String(idx)).c_str(), smartHomeRules[idx].op);
      preferences.putString(("val" + String(idx)).c_str(), smartHomeRules[idx].value);
      for(int a=0; a<3; a++) {
         preferences.putString(("a" + String(idx) + "_" + String(a)).c_str(), smartHomeRules[idx].action[a]);
         preferences.putString(("ext" + String(idx) + "_" + String(a)).c_str(), smartHomeRules[idx].extra[a]);
      }
    }
    server.sendHeader("Location", "/rules?id=" + String(idx)); server.send(302, "text/plain", "");
  });

  server.on("/exit", []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", "<html><body><h2>Rebooting...</h2></body></html>");
    delay(1000); ESP.restart();
  });

  // FIX 4: Aggressive No-Cache headers on the Captive Portal Redirect
  server.onNotFound([]() {
    server.sendHeader("Connection", "close"); 
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    server.sendHeader("Location", "http://192.168.4.1/", true); 
    server.send(302, "text/plain", ""); 
  });
}

bool connectWiFi(int timeoutSec) {
  WiFi.begin(wifi_ssid, wifi_pass);
  int attempts = timeoutSec * 2; 
  while (WiFi.status() != WL_CONNECTED && attempts > 0) { delay(500); attempts--; }
  return WiFi.status() == WL_CONNECTED;
}

void handleWiFiFailure() {
#if defined(pushbutton1Pin) && defined(pushbutton2Pin)
  while (true) {
    mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("1:Offln 2:Retry");
    mylcd.setCursor(0, 1); mylcd.print("Choose option...");
    while (true) {
      if (digitalRead(pushbutton1Pin) == LOW) {
        delay(200); offlineMode = true; WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
        mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("Offline Mode"); mylcd.setCursor(0, 1); mylcd.print("Rules Only");
        delay(2000); return;
      }
      if (digitalRead(pushbutton2Pin) == LOW) {
        delay(200); mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("Retrying 30s...");
        if (connectWiFi(30)) { WiFi.setSleep(WIFI_PS_NONE); return; } break; 
      }
      delay(50);
    }
  }
#else
  offlineMode = true; WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
#endif
}

void reconnect() { if (client.connect(("home" + String(homeNumber)).c_str())) client.subscribe(client_subscribe_all); }
void onClientConnect(WiFiEvent_t event, WiFiEventInfo_t info) { }
void count() { value++; if (!offlineMode) client.publish(client_count, String(value).c_str()); }

void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) messageTemp += (char)message[i];
  if (messageTemp == "true" || messageTemp == "1") messageTemp = "1";
  else if (messageTemp == "false" || messageTemp == "0") messageTemp = "0";

  Buzzer buzz(buzzerPin);
#ifdef SMARTHOME
  if (String(topic) == control1) { digitalWrite(LEDPin, messageTemp.toInt()); ledIsOn = messageTemp.toInt(); }
  if (String(topic) == control2) {
#ifdef doorServo    
    Dservo.write(messageTemp.toInt() == 1 ? 180 : 0); doorIsOpen = (messageTemp.toInt() == 1);
#endif    
  }
  if (String(topic) == control3) {
#ifdef windowServo    
    Wservo.write(messageTemp.toInt() == 1 ? 165 : 50); windowIsOpen = (messageTemp.toInt() == 1);
#endif    
  }
  if (String(topic) == control4 && (messageTemp == "1")) { buzz.sound(165, 100); }
  if (String(topic) == LEDcolorStrip) {
#ifdef LEDStripPin
    if (messageTemp.toInt() == 0) { stripOff(); }
    else if (messageTemp.toInt() == 1) { for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(255, 0, 0)); strip.show(); }
    else if (messageTemp.toInt() == 4) { for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 255, 0)); strip.show(); }
    else if (messageTemp.toInt() == 6) { for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 255)); strip.show(); }
#endif
  }
  if (String(topic) == control5) {
#ifdef fanPin1    
    analogWrite(fanPin1, messageTemp.toFloat() ? ((messageTemp.toFloat()) * 130 + 125) : 0); digitalWrite(fanPin2, LOW); fanIsOn = (messageTemp.toFloat() > 0);
#endif    
  }
#endif
}

#ifdef LEDStripPin
void colorWipe(uint32_t color, int wait) { for (int i = 0; i < strip.numPixels(); i++) { strip.setPixelColor(i, color); strip.show(); delay(wait); } }
void rainbow(int wait) {}
#endif

#ifdef dht11PIN
void temperature_humidity() {
  if (xht.receive(dht)) {                  
    lastTemp = dht[2] + dht[3] / 10.0; lastHum = dht[0] + dht[1] / 10.0;  
    if (!offlineMode) { client.publish(client_temperature, String(lastTemp).c_str()); client.publish(client_humidity, String(lastHum).c_str()); }
  }
}
#endif

#ifdef motionPin
void motion() {
  static bool motionOld = 1; bool motionNew = digitalRead(motionPin);
  if (motionOld != motionNew) { if (!offlineMode) client.publish(client_motion, String(motionNew).c_str()); motionOld = motionNew; }
}
#endif

#ifdef gasPin
void gas() {
  static bool gasOld = 0; bool gasNew = digitalRead(gasPin);
  if (gasOld != gasNew) { if (!offlineMode) client.publish(client_gas, String(gasNew).c_str()); gasOld = gasNew; }
}
#endif

#ifdef touchPin
void touch() {
  static int touchOld = -1; int touchNew = touchRead(touchPin);
  if (touchOld != touchNew) { if (!offlineMode) client.publish(client_touch, String(touchNew).c_str()); touchOld = touchNew; }
}
#endif

#ifdef waterLevelPin
void waterLevel() {
  static int waterLevelOld = -100; int waterLevelNew = analogRead(waterLevelPin);
  if (abs(waterLevelNew - waterLevelOld) > 50) { if (!offlineMode) client.publish(client_water, String(waterLevelNew).c_str()); waterLevelOld = waterLevelNew; }
}
#endif

#ifdef soilHumidityPin
void soilHumidity() {
  static int soilLevelOld = -100; int soilLevelNew = analogRead(soilHumidityPin);
  if (abs(soilLevelNew - soilLevelOld) > 50) { if (!offlineMode) client.publish(client_soil, String(soilLevelNew).c_str()); soilLevelOld = soilLevelNew; }
}
#endif

#ifdef RFID
String rfid() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return "";
  storedUID = mfrc522.uid; String rfidMsg = "";
  for (byte i = 0; i < storedUID.size; ++i) { rfidMsg += (storedUID.uidByte[i] < 0x10 ? "0" : ""); rfidMsg += String(storedUID.uidByte[i], HEX); }
  rfidMsg.toUpperCase(); mfrc522.PICC_HaltA(); 
  if (!offlineMode && !isConfigMode) client.publish(client_rfid, (char*)rfidMsg.c_str()); 
  return rfidMsg;
}
#endif
