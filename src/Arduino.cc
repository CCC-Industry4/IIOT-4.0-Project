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

#include <MFRC522_I2C.h> 

#include <Adafruit_NeoPixel.h>
#include <Buzzer.h>          
#include <ESP32Servo.h>

#include <LiquidCrystal_I2C.h>  

// WIP COMM HERE
int value = 0;
int homeNumber = 0; 
const char* ssid = "IT4Project";  
const char* password = "IOT12345";

const char* mqtt_server = "192.168.10.2"; 

#define SMARTHOME

char ssid_config[32];
char pass_config[32];
char mqtt_server_config[100];  

const char* wifi_ssid = nullptr;
const char* wifi_pass = nullptr;

#include "pins.h"
#define MAX_RULES 20 
#define MAX_FOBS 5 

struct Rule {
  String source;    
  String op;        
  String value;     
  String action[3]; 
  String mode[3];   
  String extra[3]; 
  bool alternate[3]; 
};

Rule smartHomeRules[MAX_RULES];
bool ruleLastState[MAX_RULES] = {false};

String knownFobs[MAX_FOBS];

Servo Dservo;
Servo Wservo;

#ifdef buzzerPin
Buzzer globalBuzz(buzzerPin);
#endif

bool fanIsOn = false;
bool ledIsOn = false;
bool doorIsOpen = false;
bool windowIsOpen = false;
bool stripIsOn = false;
bool touchIsOn = false; 

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
unsigned char dht[4] = { 0, 0, 0, 0 };  
volatile bool dhtNewData = false;

// DUAL CORE THREADING: Run the blocking DHT sensor on Core 1
void dhtTask(void *pvParameters) {
  for (;;) {
    if (xht.receive(dht)) {
      lastTemp = dht[2] + dht[3] / 10.0; 
      lastHum = dht[0] + dht[1] / 10.0;
      dhtNewData = true; 
    }
    vTaskDelay(2500 / portTICK_PERIOD_MS); 
  }
}
#endif

WebServer server(80); 
WiFiClient esp32Client; 
PubSubClient client(esp32Client);

bool reset = false;
int neighborhood;
int home;
char bruh[50];

// Non-blocking states
bool mqttSyncPending = false;
int mqttSyncStep = 0;
unsigned long lastMqttSyncTime = 0;

int currentLedMode = 0;
unsigned long lastLedUpdate = 0;
int ledStep = 0;
long ledFirstPixelHue = 0;
int ledB = 0;

bool fanIsKicking = false;
unsigned long fanKickStartTime = 0;
int fanTargetSpeed = 0;

void updateLcdWebserver();
void setupWebHandlers();
bool connectWiFi(int timeoutSec);
void handleWiFiFailure();
void reconnect();
void count();
void updateLCD();
void startConfigMode();
void onClientConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void callback(char* topic, byte* message, unsigned int length);
void executeAllActions(Rule r);
void executeAction(String target, String mode, String extra);
void executeInverseAction(String target, String mode, String extra);
void stripOff();
void evaluateRules(String event = "");
void handleLedAnimations();

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

void startConfigMode() {
  apNameStr = preferences.getString("ap_ssid", "");
  apPassStr = preferences.getString("ap_pass", "");
  int ap_uptime = preferences.getInt("ap_uptime", 0);

  if (apNameStr == "" || apPassStr == "" || ap_uptime >= 30) {
    uint16_t entropyName = esp_random() & 0xFFFF;
    uint32_t passNum = esp_random() % 100000000; 

    apNameStr = "CONFIG_" + String(entropyName, HEX);
    apNameStr.toUpperCase();
    
    char passBuf[10];
    sprintf(passBuf, "%08d", passNum);
    apPassStr = String(passBuf);

    preferences.putString("ap_ssid", apNameStr);
    preferences.putString("ap_pass", apPassStr);
    preferences.putInt("ap_uptime", 0); 
  }

  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);
  IPAddress apIP(192, 168, 4, 1);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  
  WiFi.softAP(apNameStr.c_str(), apPassStr.c_str(), 6);
  WiFi.setSleep(WIFI_PS_NONE);
  delay(100); 
  
  dnsServer.start(53, "*", apIP);
  setupWebHandlers();
  server.begin();
}

void setup() {
  Serial.begin(115200);
  
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);

  preferences.begin("smarthome", false);
  
  String storedSSID  = preferences.getString("wifi_ssid", ssid);
  String storedPASS  = preferences.getString("wifi_pass", password);
  storedSSID.toCharArray(ssid_config, 32);
  storedPASS.toCharArray(pass_config, 32);
  
  for(int i=0; i<MAX_FOBS; i++) knownFobs[i] = preferences.getString(("fob"+String(i)).c_str(), "");

  for (int i = 0; i < MAX_RULES; i++) {
    smartHomeRules[i].source = preferences.getString(("src" + String(i)).c_str(), "none");
    smartHomeRules[i].op     = preferences.getString(("op" + String(i)).c_str(), "==");
    smartHomeRules[i].value  = preferences.getString(("val" + String(i)).c_str(), "");
    for(int a=0; a<3; a++) {
      smartHomeRules[i].action[a] = preferences.getString(("a" + String(i) + "_" + String(a)).c_str(), "none");
      smartHomeRules[i].mode[a]   = preferences.getString(("m" + String(i) + "_" + String(a)).c_str(), "toggle");
      smartHomeRules[i].extra[a]  = preferences.getString(("ext" + String(i) + "_" + String(a)).c_str(), "#FFFFFF,#FFFFFF,#FFFFFF,#FFFFFF,255");
      smartHomeRules[i].alternate[a] = preferences.getBool(("alt" + String(i) + "_" + String(a)).c_str(), false);
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

#ifdef dht11PIN
  xTaskCreatePinnedToCore(
      dhtTask,       
      "DHT_Task",    
      4096,          
      NULL,          
      1,             
      NULL,          
      1);            
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
  ledcSetup(10, 20000, 8); 
  ledcAttachPin(fanPin1, 10);
#endif
#ifdef fanPin2
  pinMode(fanPin2, OUTPUT);
#endif

  reset = preferences.getBool("reset", false);
  delay(500); 

  mylcd.clear();
  mylcd.setCursor(0, 0); mylcd.print("Both Btns:Config");
  mylcd.setCursor(0, 1); mylcd.print("Btn 2:Offline 3s");
  
  unsigned long bootTime = millis();
  while (millis() - bootTime < 3000) {
#if defined(pushbutton1Pin) and defined(pushbutton2Pin)
    if (digitalRead(pushbutton1Pin) == LOW && digitalRead(pushbutton2Pin) == LOW) {
      isConfigMode = true;
      offlineMode = false;
      mylcd.clear();
      mylcd.setCursor(0, 0); mylcd.print("Config Mode");
      mylcd.setCursor(0, 1); mylcd.print("Activated!");
      delay(1000);
      break;
    } else if (digitalRead(pushbutton2Pin) == LOW) {
      offlineMode = true;
    } else {
      offlineMode = false;
    }
#endif
    delay(50);
  }

  if (offlineMode && !isConfigMode) {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    mylcd.clear();
    mylcd.setCursor(0, 0); mylcd.print("Offline Mode");
    mylcd.setCursor(0, 1); mylcd.print("Activated!");
    delay(1000);
  }

  if ((!reset || isConfigMode) && !offlineMode) {
    isConfigMode = true;
    startConfigMode();
    return; 
  }

  if (!reset) {
    wifi_ssid = ssid;
    wifi_pass = password;
  } else {
    wifi_ssid = ssid_config;
    wifi_pass = pass_config;
  }
  
  if (!offlineMode) {
    mylcd.clear();
    mylcd.setCursor(0, 0); mylcd.print("Connecting WiFi");
    
    if (!connectWiFi(10)) handleWiFiFailure();
    else WiFi.setSleep(WIFI_PS_NONE);
  }

  homeNumber = preferences.getInt("home", 0);
  neighborhood = preferences.getInt("neighborhood", 0);
  home = preferences.getInt("home", 0);
  
  if (!offlineMode && !isConfigMode) {
    String storedMQTT = preferences.getString("mqtt_ip", mqtt_server);
    storedMQTT.toCharArray(mqtt_server_config, 100);
    client.setServer(mqtt_server_config, 1883); 
    client.setCallback(callback);
    
    client.setBufferSize(1024);    
    client.setKeepAlive(60);       
    // Removed client.setSocketTimeout(2) to prevent overly strict connection aborts
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
  sprintf(client_rfid, "%s/rfid", mqttnamespace);
  sprintf(client_message, "%s/out/message", mqttnamespace);

  if (!offlineMode && !isConfigMode) {
    int attempts = 0;
    while (!client.connected() && attempts < 2) { 
      reconnect();
      delay(100); 
      attempts++;
    }
  }
  delay(100);
  if (!isConfigMode) updateLCD();
}

void loop() {
  if ((isConfigMode || !reset) && !offlineMode) {
    dnsServer.processNextRequest(); 
    updateLcdWebserver();
    server.handleClient(); 
    
    static unsigned long lastMinuteCheck = millis();
    if (millis() - lastMinuteCheck >= 60000) { 
      lastMinuteCheck = millis();
      int ap_uptime = preferences.getInt("ap_uptime", 0) + 1; 
      
      if (ap_uptime >= 30) { 
        uint16_t entropyName = esp_random() & 0xFFFF;
        uint32_t passNum = esp_random() % 100000000; 

        apNameStr = "CONFIG_" + String(entropyName, HEX);
        apNameStr.toUpperCase();
        
        char passBuf[10];
        sprintf(passBuf, "%08d", passNum);
        apPassStr = String(passBuf);

        preferences.putString("ap_ssid", apNameStr);
        preferences.putString("ap_pass", apPassStr);
        preferences.putInt("ap_uptime", 0); 

        WiFi.softAP(apNameStr.c_str(), apPassStr.c_str(), 6);
        mylcd.clear();
      } else {
        preferences.putInt("ap_uptime", ap_uptime);
      }
    }

    delay(2); 
    return;
  }
  
  if (!offlineMode && !isConfigMode && WiFi.status() != WL_CONNECTED) {
    mylcd.clear();
    mylcd.setCursor(0, 0); mylcd.print("WiFi Dropped!");
    mylcd.setCursor(0, 1); mylcd.print("Retrying 15s...");
    
    WiFi.disconnect();
    if (connectWiFi(15)) {
      mylcd.clear();
      mylcd.setCursor(0, 0); mylcd.print("Reconnected!");
      delay(1000);
      updateLCD();
    } else {
      handleWiFiFailure();
      return; 
    }
  }

  if (!offlineMode) {
    if (!client.connected()) {
      static unsigned long lastMqttRetry = 0;
      if (millis() - lastMqttRetry > 15000) {
        reconnect(); 
        lastMqttRetry = millis(); 
        if (client.connected()) updateLCD();
      }
    } else {
      client.loop(); 
      
      // Staggered MQTT Sync - Published without the 'true' retained flag
      if (mqttSyncPending && (millis() - lastMqttSyncTime > 150)) {
        lastMqttSyncTime = millis();
        switch(mqttSyncStep) {
          case 0: client.publish(control1, "null"); break;
          case 1: client.publish(control2, "null"); break;
          case 2: client.publish(control3, "null"); break;
          case 3: client.publish(control4, "null"); break;
          case 4: client.publish(control5, "null"); break;
          case 5: client.publish(LEDcolorStrip, "0"); break;
#ifdef motionPin
          case 6: client.publish(client_motion, String(digitalRead(motionPin)).c_str()); break;
#endif
#ifdef gasPin
          case 7: client.publish(client_gas, String(digitalRead(gasPin)).c_str()); break;
#endif
#ifdef pushbutton1Pin
          case 8: client.publish(client_pushbutton1, String(digitalRead(pushbutton1Pin)).c_str()); break;
#endif
#ifdef pushbutton2Pin
          case 9: client.publish(client_pushbutton2, String(digitalRead(pushbutton2Pin)).c_str()); break;
#endif
#ifdef waterLevelPin
          case 10: client.publish(client_water, String(analogRead(waterLevelPin)).c_str()); break;
#endif
#ifdef soilHumidityPin
          case 11: client.publish(client_soil, String(analogRead(soilHumidityPin)).c_str()); break;
#endif
#ifdef touchPin
          case 12: client.publish(client_touch, "65"); break;
#endif
        }
        mqttSyncStep++;
        if (mqttSyncStep > 13) mqttSyncPending = false;
      }
    }
  }

  long now = millis();
  
  // 1-Second Timer for Counters & LCD
  static unsigned long lastCountTime = 0;
  if (now - lastCountTime > 1000) { 
    lastCountTime = now;
    count();  
    updateLCD(); 
    evaluateRules(""); 
  }

  // DHT Data Publisher (triggered remotely from background task)
#ifdef dht11PIN
  if (dhtNewData) {
    dhtNewData = false;
    if (!offlineMode && client.connected()) {
      client.publish(client_temperature, String(lastTemp).c_str()); 
      client.publish(client_humidity, String(lastHum).c_str()); 
    }
  }
#endif

  // Non-blocking fan delay handler
#ifdef fanPin1
  if (fanIsKicking && millis() - fanKickStartTime >= 20) {
    ledcWrite(10, fanTargetSpeed);
    fanIsKicking = false;
  }
#endif

  // Handle non-blocking LED animations
  handleLedAnimations();

#ifdef touchPin
  touch(); 
#endif
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

#ifdef pushbutton1Pin
  static bool pb1Old = 1;
  static unsigned long pb1Db = 0;
  bool pb1New = digitalRead(pushbutton1Pin);
  if (pb1Old != pb1New && (millis() - pb1Db > 100)) {
    if (!offlineMode) client.publish(client_pushbutton1, pb1New ? "1" : "0"); 
    pb1Old = pb1New;
    pb1Db = millis();
  }
#endif

#ifdef pushbutton2Pin
  static bool pb2Old = 1;
  static unsigned long pb2Db = 0;
  bool pb2New = digitalRead(pushbutton2Pin);
  if (pb2Old != pb2New && (millis() - pb2Db > 100)) {
    if (!offlineMode) client.publish(client_pushbutton2, pb2New ? "1" : "0"); 
    pb2Old = pb2New;
    pb2Db = millis();
  }
#endif

  String e1 = btn1.update();
  if (e1 != "") {
    totalButtonPresses++;
    evaluateRules("btn1_" + e1);
  }

  String e2 = btn2.update();
  if (e2 != "") {
    totalButtonPresses++;
    evaluateRules("btn2_" + e2);
  }
  delay(10); 
}

void executeInverseAction(String target, String mode, String extra) {
  if (mode == "on") executeAction(target, "off", extra);
  else if (mode == "off") executeAction(target, "on", extra);
  else if (mode == "toggle") executeAction(target, "toggle", extra);
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
        currentVal = touchIsOn ? 1.0 : 0.0;
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
        bool isToggle = false;
        for(int a=0; a<3; a++) {
            if (r.mode[a] == "toggle") isToggle = true;
        }
        if (isToggle) {
          if (!ruleLastState[i]) executeAllActions(r); 
        } 
        else {
          executeAllActions(r);
        }
      }
    } 
    else if (ruleLastState[i] && (isContinuousBtn || (!r.source.startsWith("btn") && !r.source.startsWith("fob")))) {
       for(int a=0; a<3; a++) {
           if (r.alternate[a]) {
             executeInverseAction(r.action[a], r.mode[a], r.extra[a]);
           }
       }
    }

    if (isContinuousBtn || (!r.source.startsWith("btn") && !r.source.startsWith("fob"))) {
      ruleLastState[i] = conditionMet;
    }
  }
}

void applyCustomColors(String hexColors) {
#ifdef LEDStripPin
  int start = 0;
  int brightness = 255;
  String colors[4] = {"#000000", "#000000", "#000000", "#000000"};

  for (int c = 0; c < 5; c++) {
    if (start >= hexColors.length()) break;
    int end = hexColors.indexOf(',', start);
    if (end == -1) end = hexColors.length();
    
    if (c < 4) colors[c] = hexColors.substring(start, end);
    else if (c == 4) brightness = hexColors.substring(start, end).toInt();
    
    start = end + 1;
  }

  for (int i = 0; i < strip.numPixels(); i++) {
    String colorStr = colors[i % 4]; 
    if (colorStr.startsWith("#")) {
      long rgb = strtol(colorStr.substring(1).c_str(), NULL, 16);
      int r = ((rgb >> 16) & 0xFF) * brightness / 255;
      int g = ((rgb >> 8) & 0xFF) * brightness / 255;
      int b = (rgb & 0xFF) * brightness / 255;
      strip.setPixelColor(i, strip.Color(r, g, b));
    }
  }
  strip.show();
#endif
}

void stripOff() {
#ifdef LEDStripPin
  currentLedMode = 0; // Stop non-blocking animations
  for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 0));
  strip.show();
#endif
}

void executeAllActions(Rule r) {
    for(int a=0; a<3; a++) {
        if(r.action[a] != "none" && r.action[a] != "") {
            executeAction(r.action[a], r.mode[a], r.extra[a]);
        }
    }
}

void executeAction(String target, String mode, String extra) {
  bool toggle = (mode == "toggle");
  bool turnOn = (mode == "on");

  if (target == "fan") {
    int speed = (extra != "") ? extra.toInt() : 255;
    bool wantFan = toggle ? !fanIsOn : turnOn;
    
    if(fanIsOn != wantFan || (wantFan && fanIsOn)) {
      fanIsOn = wantFan;
#ifdef fanPin1
      if (fanIsOn) {
        ledcWrite(10, 255);
        if (speed < 255) {
          fanKickStartTime = millis();
          fanTargetSpeed = speed;
          fanIsKicking = true;
        } else {
          ledcWrite(10, speed);
        }
      } else {
        ledcWrite(10, 0); 
        fanIsKicking = false;
      }
      digitalWrite(fanPin2, LOW);
#endif
    }
  } 
  else if (target == "led") {
    bool wantLed = toggle ? !ledIsOn : turnOn;
    if(ledIsOn != wantLed) {
      ledIsOn = wantLed;
#ifdef LEDPin
      digitalWrite(LEDPin, ledIsOn ? HIGH : LOW);
#endif
    }
  }
  else if (target == "door") {
    bool wantOpen = toggle ? !doorIsOpen : turnOn;
    if(doorIsOpen != wantOpen) {
      doorIsOpen = wantOpen;
#ifdef doorServo
      Dservo.write(doorIsOpen ? 180 : 0);
#endif
    }
  }
  else if (target == "window") {
    bool wantOpen = toggle ? !windowIsOpen : turnOn;
    if(windowIsOpen != wantOpen) {
      windowIsOpen = wantOpen;
#ifdef windowServo
      Wservo.write(windowIsOpen ? 165 : 50);
#endif
    }
  }
  else if (target == "buzzer") {
#ifdef buzzerPin
    globalBuzz.sound(165, 200);
#endif
  }
  else if (target == "strip") {
    bool wantStrip = toggle ? !stripIsOn : turnOn;
    if (wantStrip) applyCustomColors(extra); else stripOff();
    stripIsOn = wantStrip;
  }
}

void updateLCD() {
  int y = value / 31536000;
  int mo = (value % 31536000) / 2592000;
  int d = (value % 2592000) / 86400;
  int h = (value % 86400) / 3600;
  int m = (value % 3600) / 60;
  int sec = value % 60;

  char topBuf[20];
  snprintf(topBuf, sizeof(topBuf), "%d:%02d:%02d:%02d:%02d:%02d", y, mo, d, h, m, sec);
  
  String topStr = String(topBuf);
  while (topStr.length() < 16) topStr += " ";

  mylcd.setCursor(0, 0); 
  mylcd.print(topStr.substring(0, 16));

  String botStr = String(bruh) + " " + String(lastTemp, 1) + "C";
  
  while(botStr.length() < 13) botStr += " "; 
  
  if (offlineMode) {
    if (botStr.length() > 13) botStr = botStr.substring(0, 13); 
    botStr += "Off";
  }
  
  while(botStr.length() < 16) botStr += " "; 
  
  mylcd.setCursor(0, 1); 
  mylcd.print(botStr.substring(0, 16));
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
    server.sendHeader("Connection", "close"); 
    
    server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    server.sendHeader("Pragma", "no-cache");
    server.sendHeader("Expires", "-1");
    
    if (server.hasArg("default")) {
      preferences.putBool("reset", false); 
      reset = false; 
      server.send(200, "text/html", "<html><body><h2>Reset to defaults. Rebooting...</h2></body></html>");
      delay(1000); ESP.restart(); return;
    }

    if (server.hasArg("action")) {
      String act = server.arg("action");
      
      int nh = server.arg("neighborhood").toInt(); 
      int hm = server.arg("home").toInt();
      // WIP COMM HERE
      if(nh < 0) nh = 0; if(nh > 100) nh = 100; 
      if(hm < 0) hm = 0; if(hm > 100) hm = 100;
      
      preferences.putInt("neighborhood", nh); 
      preferences.putInt("home", hm);
      preferences.putString("mqtt_ip", server.arg("mqttip"));
      preferences.putString("wifi_ssid", server.arg("wifissid")); 
      preferences.putString("wifi_pass", server.arg("wifipass"));
      
      if (act == "rules") {
        server.sendHeader("Location", "/rules");
        server.send(302, "text/plain", "");
        return;
      } else if (act == "fobs") {
        server.sendHeader("Location", "/fobs");
        server.send(302, "text/plain", "");
        return;
      } else if (act == "finalize") {
        preferences.putBool("reset", true);
        preferences.end();
        server.send(200, "text/html", "<html><body><h2>Settings Saved. Rebooting...</h2></body></html>");
        delay(1000); ESP.restart(); return;
      }
    }

    // WIP COMM HERE
    int storedNeighborhood = preferences.getInt("neighborhood", 0);
    int storedHome = preferences.getInt("home", 0);
    String storedMQTT = preferences.getString("mqtt_ip", mqtt_server);

    String page = "<!DOCTYPE html><html><head><meta name='viewport' content='width=device-width, initial-scale=1'><title>Config</title><style>body{font-size:18px; font-family:Arial; padding:10px;} input,button{font-size:18px; padding:6px; margin:5px 0;} h2{font-size:24px;}</style></head><body>";
    page += "<h2>Smart Home Network Configuration</h2><form method='get' action='/'>";
    
    // WIP COMM HERE
    page += "Neighborhood (0-100): <br><input type='number' name='neighborhood' value='" + String(storedNeighborhood) + "' min='0' max='100'><br>";
    
    // WIP COMM HERE
    page += "Home Number (0-100): <br><input type='number' name='home' value='" + String(storedHome) + "' min='0' max='100'><br>";
    
    page += "MQTT IP: <br><input type='text' name='mqttip' value='" + String(storedMQTT) + "'><br>";
    page += "WiFi SSID: <br><input type='text' name='wifissid' value='" + String(ssid_config) + "'><br>";
    page += "WiFi Pass: <br><input type='text' name='wifipass' value='" + String(pass_config) + "'><br><br>";
    
    page += "<button type='submit' name='action' value='finalize' style='background:#4CAF50; color:white; border:none; padding:10px; cursor:pointer;'>Finalize and Reset ESP32</button><br><br><hr><br>";
    page += "<button type='submit' name='action' value='rules' style='padding:10px; cursor:pointer;'>Go to Sandbox / Rules Config</button><br><br>";
    page += "<button type='submit' name='action' value='fobs' style='background:#2196F3; color:white; border:none; padding:10px; cursor:pointer;'>Manage Key Fobs</button></form><br>";
    
    page += "<form method='get' action='/'><input type='submit' name='default' value='Reset to Defaults' style='padding:6px; cursor:pointer;'></form></body></html>";
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
      bool isDuplicate = false;
      for(int i=0; i<MAX_FOBS; i++) {
        if (knownFobs[i] == scannedUID) isDuplicate = true;
      }
      
      if (isDuplicate) {
         mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("ALREADY PAIRED");
         delay(1500);
      } else {
        mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("SUCCESS!");
        mylcd.setCursor(0, 1); mylcd.print(scannedUID);
        knownFobs[s] = scannedUID; preferences.putString(("fob"+String(s)).c_str(), scannedUID); 
        
#ifdef buzzerPin
        globalBuzz.sound(165, 200); 
#endif
        delay(1500);
      }
    } else { mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("TIMEOUT"); delay(1500); }
    server.sendHeader("Location", "/fobs"); server.send(302, "text/plain", "");
  });

  server.on("/rules", []() {
    server.sendHeader("Connection", "close"); 
    
    // --- NEW LOGIC: Instantly create and save a default rule ---
    if (server.hasArg("id") && server.arg("id") == "new") {
      int newId = -1;
      for(int i=0; i<MAX_RULES; i++) {
        if (smartHomeRules[i].source == "none") { newId = i; break; }
      }
      if (newId != -1) {
        // Claim the slot with default settings so it is permanently occupied
        smartHomeRules[newId].source = "btn1"; 
        smartHomeRules[newId].op = "=="; 
        smartHomeRules[newId].value = "tap";
        
        for(int a=0; a<3; a++) { 
           smartHomeRules[newId].action[a] = "none"; 
           smartHomeRules[newId].mode[a] = "toggle"; 
           smartHomeRules[newId].extra[a] = "#FFFFFF,#FFFFFF,#FFFFFF,#FFFFFF,255"; 
           smartHomeRules[newId].alternate[a] = false; 
           
           // Save defaults to flash storage
           preferences.putString(("a" + String(newId) + "_" + String(a)).c_str(), "none");
           preferences.putString(("m" + String(newId) + "_" + String(a)).c_str(), "toggle");
           preferences.putString(("ext" + String(newId) + "_" + String(a)).c_str(), "#FFFFFF,#FFFFFF,#FFFFFF,#FFFFFF,255");
           preferences.putBool(("alt" + String(newId) + "_" + String(a)).c_str(), false);
        }
        preferences.putString(("src" + String(newId)).c_str(), "btn1");
        preferences.putString(("op" + String(newId)).c_str(), "==");
        preferences.putString(("val" + String(newId)).c_str(), "tap");

        // Redirect seamlessly to edit the newly created rule
        server.sendHeader("Location", "/rules?id=" + String(newId)); 
        server.send(302, "text/plain", "");
        return;
      }
    }

    int editId = server.hasArg("id") ? server.arg("id").toInt() : 0;
    if (editId < 0) editId = 0; if (editId >= MAX_RULES) editId = MAX_RULES - 1;

    String page;
    page.reserve(10000); 
    
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
            
            "  var isMomentary = (src.indexOf('fob') === 0 || ((src === 'btn1' || src === 'btn2') && vBtn.value !== 'raw'));"
            "  for(var i=0; i<3; i++) {"
            "    var altCb = document.getElementById('altWrap_' + i);"
            "    if (altCb) altCb.style.display = isMomentary ? 'none' : 'inline-block';"
            "  }"

            "  if(src === 'btn1' || src === 'btn2') {"
            "    opC.style.display = 'none'; vBtn.style.display = 'inline-block'; vBtn.disabled = false;"
            "  } else if(src.indexOf('fob') === 0) {"
            "    opC.style.display = 'none';"
            "  } else if(src === 'motion' || src === 'gas' || src === 'touch') {"
            "    opC.style.display = 'none';" 
            "    vBool.style.display = 'inline-block'; vBool.disabled = false;"
            "  } else {"
            "    opC.style.display = 'inline-block'; vIn.style.display = 'inline-block'; vIn.disabled = false; vIn.type = 'number'; vIn.step = 'any';"
            "  }"
            "  for(var i=0; i<3; i++) updateActionUI(i);"
            "}"
            
            "function updateActionUI(idx) {"
            "  var act = document.getElementById('actSelect_' + idx).value;"
            "  var modSel = document.getElementById('modeSel_' + idx);"
            "  var ledDiv = document.getElementById('ledConfig_' + idx);"
            
            "  if (act === 'none' || act === 'buzzer') modSel.style.display = 'none';"
            "  else modSel.style.display = 'inline-block';"
            
            "  if(act === 'strip') { if(ledDiv) ledDiv.style.display = 'block'; }"
            "  else { if (ledDiv) ledDiv.style.display = 'none'; }"
            
            "  var optOn = modSel.options[1]; var optOff = modSel.options[2];"
            "  if (act === 'window' || act === 'door') { optOn.text = 'Open'; optOff.text = 'Close'; }"
            "  else { optOn.text = 'Turn ON'; optOff.text = 'Turn OFF'; }"
            "}"
            "window.onload = updateUI;"
            "</script></head><body><h2>Rule Sandbox Configuration</h2>";

    page += "<form method='get' action='/rules'>Select Rule to Edit: <select name='id' onchange='this.form.submit()'>";
    int nextEmpty = -1;
    for(int i=0; i<MAX_RULES; i++){
      // If the rule has been claimed, show it in the list
      if(smartHomeRules[i].source != "none") {
        page += "<option value='" + String(i) + "'" + (i == editId ? " selected" : "") + ">Rule " + String(i+1) + "</option>";
      } else if (nextEmpty == -1) {
        nextEmpty = i;
      }
    }
    
    // Pass 'new' to trigger the background creation logic above
    if (nextEmpty != -1) {
      page += "<option value='new'>+ Add New Rule</option>";
    }
    page += "</select></form><hr>";

    auto getOpt = [](String val, String label, String savedVal) { return "<option value='" + val + "'" + (savedVal == val ? " selected" : "") + ">" + label + "</option>"; };

    page += "<form method='get' action='/saverule' class='box'><input type='hidden' name='id' value='" + String(editId) + "'>";
    page += "<b>Editing Rule " + String(editId+1) + "</b><br><br>IF Source: <select id='srcSelect' name='src' onchange='updateUI()'>";
    page += getOpt("none", "None", smartHomeRules[editId].source) + getOpt("btn1", "Button 1", smartHomeRules[editId].source) + getOpt("btn2", "Button 2", smartHomeRules[editId].source);
    page += getOpt("fob_any", "Any Registered Fob", smartHomeRules[editId].source);
    
    for(int i=0; i<MAX_FOBS; i++) {
        if (knownFobs[i] != "") {
             page += getOpt("fob" + String(i+1), "Fob Slot " + String(i+1), smartHomeRules[editId].source);
        }
    }
    
    page += getOpt("temp", "Temperature", smartHomeRules[editId].source) + getOpt("hum", "Humidity", smartHomeRules[editId].source) + getOpt("touch", "Touch Sensor", smartHomeRules[editId].source);
    page += getOpt("motion", "Motion Sensor", smartHomeRules[editId].source) + getOpt("gas", "Gas Sensor", smartHomeRules[editId].source) + getOpt("water", "Water Level", smartHomeRules[editId].source) + getOpt("soil", "Soil Humidity", smartHomeRules[editId].source);
    page += "</select><br>";

    page += "<span id='opContainer'>Operator: <select id='opSelect' name='op'>" + getOpt("==", "Is (==)", smartHomeRules[editId].op) + getOpt(">", "Greater (>)", smartHomeRules[editId].op) + getOpt("<", "Less (<)", smartHomeRules[editId].op) + "</select><br></span>";

    String curVal = smartHomeRules[editId].value;
    page += "Value: <input type='text' id='valInput' name='val_num' value='" + curVal + "' placeholder='...'>";
    page += "<select id='valSelBtn' name='val_btn' onchange='updateUI()' style='display:none;'>" + getOpt("tap", "Single Tap", curVal) + getOpt("double", "Double Tap", curVal) + getOpt("hold", "Hold", curVal) + getOpt("raw", "While Holding", curVal) + "</select>";
    page += "<select id='valSelBool' name='val_bool' style='display:none;'>" + getOpt("1", "True", curVal) + getOpt("0", "False", curVal) + "</select><br><br><hr style='border:1px solid #ddd;'>";

    for(int a=0; a<3; a++) {
        page += "<b>THEN Action " + String(a+1) + ":</b> <select id='actSelect_" + String(a) + "' name='a" + String(a) + "' onchange='updateActionUI(" + String(a) + ")'>";
        page += getOpt("none", "None", smartHomeRules[editId].action[a]);
        page += getOpt("fan", "Fan", smartHomeRules[editId].action[a]);
        page += getOpt("led", "LED", smartHomeRules[editId].action[a]);
        page += getOpt("door", "Door", smartHomeRules[editId].action[a]);
        page += getOpt("window", "Window", smartHomeRules[editId].action[a]);
        page += getOpt("buzzer", "Buzzer Beep", smartHomeRules[editId].action[a]);
        page += getOpt("strip", "Custom LED Strip", smartHomeRules[editId].action[a]);
        page += "</select> ";

        page += "<select id='modeSel_" + String(a) + "' name='m" + String(a) + "'>";
        page += getOpt("toggle", "Toggle", smartHomeRules[editId].mode[a]);
        page += getOpt("on", "Turn ON", smartHomeRules[editId].mode[a]);
        page += getOpt("off", "Turn OFF", smartHomeRules[editId].mode[a]);
        page += "</select> ";

        String checked = smartHomeRules[editId].alternate[a] ? "checked" : "";
        page += "<span id='altWrap_" + String(a) + "' style='font-size:14px; margin-left:10px;'><input type='checkbox' name='alt" + String(a) + "' value='1' " + checked + "> Revert when false</span><br>";

        String hexColor = smartHomeRules[editId].extra[a];
        String colors[4] = {"#FFFFFF", "#FFFFFF", "#FFFFFF", "#FFFFFF"};
        String stripBright = "255";

        if (smartHomeRules[editId].action[a] == "strip") {
            if (hexColor.indexOf(',') != -1) {
                int start = 0;
                for(int c = 0; c < 5; c++) {
                    if (start >= hexColor.length()) break;
                    int end = hexColor.indexOf(',', start);
                    if (end == -1) end = hexColor.length();
                    
                    if (c < 4) colors[c] = hexColor.substring(start, end);
                    if (c == 4) stripBright = hexColor.substring(start, end);
                    start = end + 1;
                }
            } else if (hexColor != "") {
                colors[0] = hexColor; 
            }
        } 

        page += "<div id='ledConfig_" + String(a) + "' style='display:none; margin-top:5px; margin-bottom:15px; padding:10px; background:#ddd; border-radius:5px;'>";
        page += "<b>LED Colors (1-4):</b><br>";
        
        for (int c = 1; c <= 4; c++) {
            page += "<input type='color' name='hex" + String(a) + "_" + String(c) + "' value='" + colors[c-1] + "' title='LED " + String(c) + "' style='width:45px; height:40px; padding:0px; margin-right:5px; cursor:pointer;'>";
        }
        page += "<br><br><b>Brightness (0-255):</b><br>";
        page += "<input type='range' name='stripbright" + String(a) + "' min='0' max='255' value='" + stripBright + "' oninput='document.getElementById(\"sbVal_" + String(a) + "\").innerText=this.value;'> ";
        page += "<span id='sbVal_" + String(a) + "'>" + stripBright + "</span>";
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
        for(int a=0; a<3; a++) { smartHomeRules[idx].action[a] = "none"; smartHomeRules[idx].mode[a] = "toggle"; smartHomeRules[idx].extra[a] = "#FFFFFF,#FFFFFF,#FFFFFF,#FFFFFF,255"; smartHomeRules[idx].alternate[a] = false; }
      } else {
        String s = server.arg("src"); String o = server.arg("op"); 
        
        String v = "";
        if (server.hasArg("val_btn")) v = server.arg("val_btn");
        else if (server.hasArg("val_bool")) v = server.arg("val_bool");
        else if (server.hasArg("val_num")) v = server.arg("val_num");

        if (s == "btn1" || s == "btn2" || s.startsWith("fob")) o = "=="; 
        else if (s == "motion" || s == "gas" || s == "touch") { o = "=="; if (v != "0" && v != "1") v = "1"; } 
        else if (s != "none") { float checkNum = v.toFloat(); if (checkNum == 0.0 && v != "0" && v != "0.0") v = "0"; }
        smartHomeRules[idx].source = s; smartHomeRules[idx].op = o; smartHomeRules[idx].value = v;

        for(int a=0; a<3; a++) {
            smartHomeRules[idx].action[a] = server.arg("a" + String(a));
            smartHomeRules[idx].mode[a] = server.arg("m" + String(a));
            
            if (smartHomeRules[idx].action[a] == "strip") {
              String c1 = server.arg("hex" + String(a) + "_1");
              String c2 = server.arg("hex" + String(a) + "_2");
              String c3 = server.arg("hex" + String(a) + "_3");
              String c4 = server.arg("hex" + String(a) + "_4");
              String sb = server.arg("stripbright" + String(a));
              if(sb == "") sb = "255";
              smartHomeRules[idx].extra[a] = c1 + "," + c2 + "," + c3 + "," + c4 + "," + sb;
            } else {
              smartHomeRules[idx].extra[a] = "";
            }
            
            smartHomeRules[idx].alternate[a] = server.hasArg("alt" + String(a));
        }
      }
      preferences.putString(("src" + String(idx)).c_str(), smartHomeRules[idx].source);
      preferences.putString(("op" + String(idx)).c_str(), smartHomeRules[idx].op);
      preferences.putString(("val" + String(idx)).c_str(), smartHomeRules[idx].value);
      for(int a=0; a<3; a++) {
         preferences.putString(("a" + String(idx) + "_" + String(a)).c_str(), smartHomeRules[idx].action[a]);
         preferences.putString(("m" + String(idx) + "_" + String(a)).c_str(), smartHomeRules[idx].mode[a]);
         preferences.putString(("ext" + String(idx) + "_" + String(a)).c_str(), smartHomeRules[idx].extra[a]);
         preferences.putBool(("alt" + String(idx) + "_" + String(a)).c_str(), smartHomeRules[idx].alternate[a]);
      }
    }
    server.sendHeader("Location", "/rules?id=" + String(idx)); server.send(302, "text/plain", "");
  });

  server.on("/exit", []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", "<html><body><h2>Rebooting...</h2></body></html>");
    delay(1000); ESP.restart();
  });

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
    mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("WiFi Failed!");
    mylcd.setCursor(0, 1); mylcd.print("1:Offln 2:Config");
    while (true) {
      if (digitalRead(pushbutton1Pin) == LOW) {
        delay(200); offlineMode = true; WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
        mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("Offline Mode"); mylcd.setCursor(0, 1); mylcd.print("Rules Only");
        delay(2000); return;
      }
      if (digitalRead(pushbutton2Pin) == LOW) {
        delay(200); mylcd.clear(); mylcd.setCursor(0, 0); mylcd.print("Starting Config.");
        isConfigMode = true;
        offlineMode = false;
        startConfigMode();
        return; 
      }
      delay(50);
    }
  }
#else
  offlineMode = true; WiFi.disconnect(true); WiFi.mode(WIFI_OFF);
#endif
}

void reconnect() { 
  // RESTORED: Exact strict Client ID so Ignition populates the website correctly
  String clientId = "home" + String(homeNumber);
  
  if (client.connect(clientId.c_str())) {
    client.subscribe(client_subscribe_all); 
    
    // Begin staggered publishing of states instead of bulk-sending
    mqttSyncPending = true;
    mqttSyncStep = 0;
    lastMqttSyncTime = millis();
  }
}

void onClientConnect(WiFiEvent_t event, WiFiEventInfo_t info) { }

void count() { 
  value++; 
  if (!offlineMode) {
    client.publish(client_count, String(value).c_str()); 
  }
}

void callback(char* topic, byte* message, unsigned int length) {
  String messageTemp;
  for (int i = 0; i < length; i++) messageTemp += (char)message[i];
  if (messageTemp == "true" || messageTemp == "1") messageTemp = "1";
  else if (messageTemp == "false" || messageTemp == "0") messageTemp = "0";

#ifdef SMARTHOME
  if (String(topic) == control1) { 
    digitalWrite(LEDPin, messageTemp.toInt()); 
    ledIsOn = messageTemp.toInt(); 
  }
  if (String(topic) == control2) {
#ifdef doorServo    
    bool wantOpen = (messageTemp.toInt() == 1);
    if(doorIsOpen != wantOpen) {
      doorIsOpen = wantOpen;
      Dservo.write(doorIsOpen ? 180 : 0); 
    }
#endif    
  }
  if (String(topic) == control3) {
#ifdef windowServo    
    bool wantOpen = (messageTemp.toInt() == 1);
    if(windowIsOpen != wantOpen) {
      windowIsOpen = wantOpen;
      Wservo.write(windowIsOpen ? 165 : 50); 
    }
#endif    
  }
  if (String(topic) == control4 && (messageTemp == "1")) { 
#ifdef buzzerPin
    globalBuzz.sound(165, 100); 
#endif
  }
  if (String(topic) == LEDcolorStrip) {
#ifdef LEDStripPin
    int msg = messageTemp.toInt();
    if (msg == 0) {
      stripOff();
    } else {
      // Set non-blocking animation state
      currentLedMode = msg;
      ledStep = 0;
      ledFirstPixelHue = 0;
      ledB = 0;
      lastLedUpdate = millis();
    }
#endif
  }
  if (String(topic) == control5) {
#ifdef fanPin1    
    float fSpeed = messageTemp.toFloat();
    if (fSpeed > 0) {
      ledcWrite(10, 255); 
      fanKickStartTime = millis();
      fanTargetSpeed = (fSpeed * 130 + 125);
      fanIsKicking = true;
    } else {
      ledcWrite(10, 0);
      fanIsKicking = false;
    }
    digitalWrite(fanPin2, LOW); 
    fanIsOn = (fSpeed > 0);
#endif    
  }
#endif
}

#ifdef LEDStripPin
// Non-blocking LED animations executed inside loop()
void handleLedAnimations() {
  if (currentLedMode == 0) return;

  if (currentLedMode >= 1 && currentLedMode <= 8) {
    if (millis() - lastLedUpdate >= 50) {
      lastLedUpdate = millis();
      uint32_t color;
      switch (currentLedMode) {
        case 1: color = strip.Color(255, 0, 0); break;
        case 2: color = strip.Color(200, 100, 0); break;
        case 3: color = strip.Color(200, 200, 0); break;
        case 4: color = strip.Color(0, 255, 0); break;
        case 5: color = strip.Color(0, 100, 255); break;
        case 6: color = strip.Color(0, 0, 255); break;
        case 7: color = strip.Color(100, 0, 255); break;
        case 8: color = strip.Color(255, 255, 255); break;
      }
      if (ledStep < strip.numPixels()) {
        strip.setPixelColor(ledStep, color);
        strip.show();
        ledStep++;
      }
    }
  } else if (currentLedMode == 9) { // rainbow
    if (millis() - lastLedUpdate >= 10) {
      lastLedUpdate = millis();
      for (int i = 0; i < strip.numPixels(); i++) {
        int pixelHue = ledFirstPixelHue + (i * 65536L / strip.numPixels());
        strip.setPixelColor(i, strip.gamma32(strip.ColorHSV(pixelHue)));
      }
      strip.show();
      ledFirstPixelHue += 256;
      if (ledFirstPixelHue >= 5 * 65536) ledFirstPixelHue = 0;
    }
  } else if (currentLedMode == 10) { // theaterChaseRainbow
    if (millis() - lastLedUpdate >= 50) {
      lastLedUpdate = millis();
      strip.clear();
      for (int c = ledB; c < strip.numPixels(); c += 3) {
        int hue = ledFirstPixelHue + c * 65536L / strip.numPixels();
        strip.setPixelColor(c, strip.gamma32(strip.ColorHSV(hue)));
      }
      strip.show();
      ledB++;
      if (ledB >= 3) {
        ledB = 0;
        ledFirstPixelHue += 65536 / 90;
      }
    }
  }
}
#endif

#ifdef motionPin
void motion() {
  // HARD THROTTLE: Max 1 check/publish per 250ms
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 250) return;
  lastCheck = millis();
  
  static bool motionOld = 1; bool motionNew = digitalRead(motionPin);
  if (motionOld != motionNew) { 
    if (!offlineMode) client.publish(client_motion, String(motionNew).c_str()); 
    motionOld = motionNew; 
  }
}
#endif

#ifdef gasPin
void gas() {
  // HARD THROTTLE: Max 1 check/publish per 500ms
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 500) return;
  lastCheck = millis();
  
  static bool gasOld = 0; bool gasNew = digitalRead(gasPin);
  if (gasOld != gasNew) { 
    if (!offlineMode) client.publish(client_gas, String(gasNew).c_str()); 
    gasOld = gasNew; 
  }
}
#endif

#ifdef touchPin
void touch() {
  // HARD THROTTLE: Max 1 check/publish per 250ms
  static unsigned long lastTouchPoll = 0;
  if (millis() - lastTouchPoll < 250) return;
  lastTouchPoll = millis();

  int touchNewRaw = touchRead(touchPin);
  
  float fuzzy_touched = 0.0;
  if (touchNewRaw <= 20) fuzzy_touched = 1.0;
  else if (touchNewRaw >= 60) fuzzy_touched = 0.0;
  else fuzzy_touched = (60.0 - touchNewRaw) / 40.0;
  
  static float touchConfidence = 0.0;
  touchConfidence = (touchConfidence * 0.6) + (fuzzy_touched * 0.4); 

  static bool ruleTouchState = false;
  bool stateChanged = false;

  if (!ruleTouchState && touchConfidence > 0.75) {
    ruleTouchState = true;
    stateChanged = true;
  } 
  else if (ruleTouchState && touchConfidence < 0.25) {
    ruleTouchState = false;
    stateChanged = true;
  }
  
  if (stateChanged) {
    touchIsOn = ruleTouchState;
    evaluateRules(ruleTouchState ? "touch_on" : "touch_off");
    
    if (!offlineMode) {
      client.publish(client_touch, String(touchNewRaw).c_str());
    }
  }
}
#endif

#ifdef waterLevelPin
void waterLevel() {
  // HARD THROTTLE: Max 1 check/publish per 1000ms
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();
  
  static int waterLevelOld = -1000; int waterLevelNew = analogRead(waterLevelPin);
  if (abs(waterLevelNew - waterLevelOld) > 50) { 
    if (!offlineMode) client.publish(client_water, String(waterLevelNew).c_str()); 
    waterLevelOld = waterLevelNew; 
  }
}
#endif

#ifdef soilHumidityPin
void soilHumidity() {
  // HARD THROTTLE: Max 1 check/publish per 1000ms
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 1000) return;
  lastCheck = millis();
  
  static int soilLevelOld = -1000; int soilLevelNew = analogRead(soilHumidityPin);
  if (abs(soilLevelNew - soilLevelOld) > 50) { 
    if (!offlineMode) client.publish(client_soil, String(soilLevelNew).c_str()); 
    soilLevelOld = soilLevelNew; 
  }
}
#endif

#ifdef RFID
String rfid() {
  static unsigned long lastCheck = 0;
  if (millis() - lastCheck < 500) return "";
  lastCheck = millis();

  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) return "";
  storedUID = mfrc522.uid; String rfidMsg = "";
  for (byte i = 0; i < storedUID.size; ++i) { rfidMsg += (storedUID.uidByte[i] < 0x10 ? "0" : ""); rfidMsg += String(storedUID.uidByte[i], HEX); }
  rfidMsg.toUpperCase(); mfrc522.PICC_HaltA(); 
  if (!offlineMode && !isConfigMode) client.publish(client_rfid, (char*)rfidMsg.c_str()); 
  return rfidMsg;
}
#endif
