#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <DNSServer.h> 

#include <Wire.h>
#include "xht11.h"          
#include <SPI.h>
#include <MFRC522v2.h>
#include <MFRC522DriverI2C.h>
#include <MFRC522Debug.h>

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

struct Rule {
  String source;    
  String op;        
  String value;     
  String action;
  String extra;
};

Rule smartHomeRules[MAX_RULES];
bool ruleLastState[MAX_RULES] = {false};

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
const uint8_t customAddress = 0x28;
TwoWire& customI2C = Wire;
MFRC522DriverI2C driver{ customAddress, customI2C };  
MFRC522 mfrc522{ driver };                            
MFRC522::Uid storedUID;
#endif

#ifdef dht11PIN
xht11 xht(dht11PIN);
#endif

unsigned char dht[4] = { 0, 0, 0, 0 };  
WiFiServer server(80);
WiFiClient esp32Client;
PubSubClient client(esp32Client);

bool reset = false;
int neighborhood;
int home;
char bruh[50];

void webserver();
bool connectWiFi(int timeoutSec);
void handleWiFiFailure();
void reconnect();
void count();
void updateLCD();
void onClientConnect(WiFiEvent_t event, WiFiEventInfo_t info);
void callback(char* topic, byte* message, unsigned int length);
void executeAction(Rule r);
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

// WIP COMM HERE
void setup() {
  Serial.begin(115200);
  preferences.begin("smarthome", false);
  
  for (int i = 0; i < MAX_RULES; i++) {
    smartHomeRules[i].source = preferences.getString(("src" + String(i)).c_str(), "none");
    smartHomeRules[i].op     = preferences.getString(("op" + String(i)).c_str(), "==");
    smartHomeRules[i].value  = preferences.getString(("val" + String(i)).c_str(), "");
    smartHomeRules[i].action = preferences.getString(("a" + String(i)).c_str(), "none");
    smartHomeRules[i].extra  = preferences.getString(("ext" + String(i)).c_str(), "");
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

  mylcd.init();
  mylcd.backlight();

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
  mylcd.setCursor(0, 0);
  mylcd.print("Press both btns");
  mylcd.setCursor(0, 1);
  mylcd.print("for Config (3s)");
  
  unsigned long bootTime = millis();
  while (millis() - bootTime < 3000) {
#if defined(pushbutton1Pin) && defined(pushbutton2Pin)
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
    WiFi.softAP(apNameStr.c_str(), apPassStr.c_str());
    delay(500); 
    
    dnsServer.start(53, "*", IPAddress(192, 168, 4, 1));
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
  mylcd.setCursor(0, 0);
  mylcd.print("Connecting WiFi");
  
  if (!connectWiFi(10)) {
    handleWiFiFailure();
  }

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

#ifdef RFID
  mfrc522.PCD_Init();
#endif

  if (!offlineMode) {
    reconnect();
    delay(500);
    client.publish(LEDcolorStrip, "0");
  }

  delay(500);
  updateLCD();
}

// WIP COMM HERE
void loop() {
  static long lastMsg = 0;
  
  if (isConfigMode || !reset) {
    dnsServer.processNextRequest(); 
    webserver();
    delay(10); 
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
    evaluateRules("rfid_" + currentRfid);
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
    else if (r.source == "btn1" || r.source == "btn2" || r.source == "rfid") {
      String expectedEvent = r.source + "_" + r.value;
      if (triggerEvent == expectedEvent) conditionMet = true;
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
      if (!isContinuousBtn && (r.source == "btn1" || r.source == "btn2" || r.source == "rfid")) {
        executeAction(r);
      } 
      else {
        bool isToggleAction = (r.action.indexOf("toggle") >= 0);
        if (isToggleAction) {
          if (!ruleLastState[i]) executeAction(r); 
        } 
        else if (r.action == "strip_flash") {
          if (!ruleLastState[i]) executeAction(r); 
        }
        else {
          executeAction(r);
        }
      }
    } 
    else if (ruleLastState[i] && (isContinuousBtn || (r.source != "btn1" && r.source != "btn2" && r.source != "rfid"))) {
       if (r.action == "strip_momentary") stripOff();
    }

    if (isContinuousBtn || (r.source != "btn1" && r.source != "btn2" && r.source != "rfid")) {
      ruleLastState[i] = conditionMet;
    }
  }
}

void applyCustomColors(Rule r) {
#ifdef LEDStripPin
  int colors[12] = {0};
  int startIdx = 0;
  for(int i=0; i<12; i++) {
      int commaIdx = r.extra.indexOf(',', startIdx);
      if (commaIdx == -1) commaIdx = r.extra.length();
      colors[i] = r.extra.substring(startIdx, commaIdx).toInt();
      startIdx = commaIdx + 1;
  }
  for (int i = 0; i < 4; i++) {
    if (i < strip.numPixels()) {
      strip.setPixelColor(i, strip.Color(colors[i*3], colors[i*3+1], colors[i*3+2]));
    }
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

// WIP COMM HERE
void executeAction(Rule r) {
  Buzzer buzz(buzzerPin);
  String action = r.action;
  
  if (action == "fan_toggle") {
    fanIsOn = !fanIsOn;
#ifdef fanPin1
    analogWrite(fanPin1, fanIsOn ? 255 : 0);
    digitalWrite(fanPin2, LOW);
#endif
  } 
  else if (action == "fan_on") {
    fanIsOn = true;
#ifdef fanPin1
    analogWrite(fanPin1, 255);
    digitalWrite(fanPin2, LOW);
#endif
  }
  else if (action == "fan_off") {
    fanIsOn = false;
#ifdef fanPin1
    analogWrite(fanPin1, 0);
    digitalWrite(fanPin2, LOW);
#endif
  }
  else if (action == "led_toggle") {
    ledIsOn = !ledIsOn;
#ifdef LEDPin
    digitalWrite(LEDPin, ledIsOn ? HIGH : LOW);
#endif
  }
  else if (action == "door_toggle") {
    doorIsOpen = !doorIsOpen;
#ifdef doorServo
    Dservo.write(doorIsOpen ? 180 : 0);
#endif
  }
  else if (action == "door_open") {
    doorIsOpen = true;
#ifdef doorServo
    Dservo.write(180);
#endif
  }
  else if (action == "door_close") {
    doorIsOpen = false;
#ifdef doorServo
    Dservo.write(0);
#endif
  }
  else if (action == "window_toggle") {
    windowIsOpen = !windowIsOpen;
#ifdef windowServo
    Wservo.write(windowIsOpen ? 165 : 50);
#endif
  }
  else if (action == "window_open") {
    windowIsOpen = true;
#ifdef windowServo
    Wservo.write(165);
#endif
  }
  else if (action == "window_close") {
    windowIsOpen = false;
#ifdef windowServo
    Wservo.write(50);
#endif
  }
  else if (action == "buzzer_beep") {
    buzz.sound(165, 200);
  }
  else if (action == "strip_toggle") {
    stripIsOn = !stripIsOn;
    if (stripIsOn) applyCustomColors(r);
    else stripOff();
  }
  else if (action == "strip_momentary") {
    stripIsOn = true;
    applyCustomColors(r);
  }
  else if (action == "strip_flash") {
    applyCustomColors(r);
    delay(100);
    stripOff();
  }
}

void updateLCD() {
  auto getDisplayString = [](int opt, int length) {
    String s = "";
    if (opt == 0) {
      if (offlineMode) {
        s = "Offline ";
      } else {
        s = String(bruh) + " ";
      }
      int d = value / 86400;
      int h = (value % 86400) / 3600;
      int m = (value % 3600) / 60;
      int sec = value % 60;
      char buf[20];
      sprintf(buf, "%d:%02d:%02d:%02d", d, h, m, sec);
      s += String(buf);
    }
    else if (opt == 1) s = String(lastTemp, 1) + "C";
    else if (opt == 2) s = String(lastHum, 1) + "%";
    else if (opt == 3) s = String(totalButtonPresses);
    else if (opt == 4) s = String(value);
    else if (opt == 5) s = "";
    
    while (s.length() < length) s += " ";
    return s.substring(0, length);
  };

  mylcd.setCursor(0, 0); mylcd.print(getDisplayString(lcdTop, 16));
  mylcd.setCursor(0, 1); mylcd.print(getDisplayString(lcdBL, 8));
  mylcd.setCursor(8, 1); mylcd.print(getDisplayString(lcdBR, 8));
}

// WIP COMM HERE
void webserver() {
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

  WiFiClient webClient = server.available();
  if (!webClient) return;

  auto getParam = [](String key, const String& fullString) {
    String match1 = "?" + key + "=";
    String match2 = "&" + key + "=";
    int idx = fullString.indexOf(match1);
    if (idx < 0) idx = fullString.indexOf(match2);
    if (idx < 0) return String("");
    
    int start = idx + key.length() + 2; 
    int end = fullString.indexOf("&", start);
    if (end < 0) end = fullString.indexOf(" ", start);
    if (end < 0) end = fullString.length();
    
    String part = fullString.substring(start, end);
    part.trim(); part.replace("+", " ");
    return part;
  };

  String readString = "";
  String currentLine = "";
  
  unsigned long clientTimeout = millis();

  while (webClient.connected() && (millis() - clientTimeout < 2000)) {
    if (webClient.available()) {
      char c = webClient.read();
      readString += c;
      clientTimeout = millis(); 

      if (c == '\n') {
        if (currentLine.length() == 0) {
          
          if (readString.indexOf("GET /rules") >= 0) {
            int editId = 0;
            if (readString.indexOf("id=") >= 0) {
              editId = getParam("id", readString).toInt();
            }
            if (editId < 0) editId = 0;
            if (editId >= MAX_RULES) editId = MAX_RULES - 1;

            webClient.println("HTTP/1.1 200 OK");
            webClient.println("Content-Type: text/html");
            webClient.println("Connection: close");
            webClient.println();
            
            webClient.println("<html><head><style>body{font-size:18px; font-family:Arial;} select,input,button{font-size:16px; padding:4px; margin-bottom:8px;} .box{background:#f4f4f4; padding:10px; border-radius:8px;}</style>");
            
            webClient.println("<script>");
            webClient.println("function updateUI() {");
            webClient.println("  var src = document.getElementById('srcSelect').value;");
            webClient.println("  var op = document.getElementById('opSelect');");
            webClient.println("  var opC = document.getElementById('opContainer');");
            webClient.println("  var vIn = document.getElementById('valInput');");
            webClient.println("  var vBtn = document.getElementById('valSelBtn');");
            webClient.println("  var vBool = document.getElementById('valSelBool');");
            webClient.println("  var pBtn = document.getElementById('pairBtn');");
            
            webClient.println("  vIn.style.display = 'none'; vIn.disabled = true;");
            webClient.println("  vBtn.style.display = 'none'; vBtn.disabled = true;");
            webClient.println("  vBool.style.display = 'none'; vBool.disabled = true;");
            webClient.println("  pBtn.style.display = 'none';");
            
            webClient.println("  if(src === 'btn1' || src === 'btn2') {");
            webClient.println("    opC.style.display = 'none';");
            webClient.println("    vBtn.style.display = 'inline-block'; vBtn.disabled = false;");
            webClient.println("  } else if(src === 'rfid') {");
            webClient.println("    opC.style.display = 'none';");
            webClient.println("    vIn.style.display = 'inline-block'; vIn.disabled = false; vIn.type = 'text';");
            webClient.println("    pBtn.style.display = 'inline-block';");
            webClient.println("  } else if(src === 'motion' || src === 'gas') {");
            webClient.println("    opC.style.display = 'inline-block';");
            webClient.println("    if(op.value !== '==') op.value = '==';");
            webClient.println("    vBool.style.display = 'inline-block'; vBool.disabled = false;");
            webClient.println("  } else {");
            webClient.println("    opC.style.display = 'inline-block';");
            webClient.println("    vIn.style.display = 'inline-block'; vIn.disabled = false; vIn.type = 'number'; vIn.step = 'any';");
            webClient.println("  }");
            webClient.println("  updateActionUI();");
            webClient.println("}");
            webClient.println("function updateActionUI() {");
            webClient.println("  var act = document.getElementById('actSelect').value;");
            webClient.println("  var ledDiv = document.getElementById('ledConfig');");
            webClient.println("  if(act.indexOf('strip_') === 0) {");
            webClient.println("    ledDiv.style.display = 'block';");
            webClient.println("  } else {");
            webClient.println("    ledDiv.style.display = 'none';");
            webClient.println("  }");
            webClient.println("}");
            webClient.println("window.onload = updateUI;");
            webClient.println("</script></head><body>");
            
            webClient.println("<h2>Rule Sandbox Configuration</h2>");

            webClient.println("<form method='get' action='/rules'>");
            webClient.println("Select Rule to Edit: <select name='id' onchange='this.form.submit()'>");
            
            int firstEmpty = -1;
            for(int i=0; i<MAX_RULES; i++){
              if(smartHomeRules[i].source != "none") {
                webClient.print("<option value='"); webClient.print(i); webClient.print("'");
                if(i == editId) webClient.print(" selected");
                webClient.print(">Rule "); webClient.print(i+1); webClient.println("</option>");
              } else if (firstEmpty == -1) {
                firstEmpty = i;
              }
            }
            
            if (firstEmpty != -1 && smartHomeRules[editId].source == "none") {
              webClient.print("<option value='"); webClient.print(editId); webClient.print("' selected");
              webClient.print(">+ New Rule (Slot "); webClient.print(editId+1); webClient.println(")</option>");
            } else if (firstEmpty != -1) {
              webClient.print("<option value='"); webClient.print(firstEmpty); 
              webClient.println("'>+ Add New Rule</option>");
            }

            webClient.println("</select></form><hr>");

            webClient.println("<form method='get' action='/saverule' class='box'>");
            webClient.print("<input type='hidden' name='id' value='"); webClient.print(editId); webClient.println("'>");
            
            auto printOption = [&](String val, String label, String savedVal) {
              webClient.print("<option value='"); webClient.print(val); webClient.print("'");
              if (savedVal == val) webClient.print(" selected");
              webClient.print(">"); webClient.print(label); webClient.println("</option>");
            };

            webClient.print("<b>Editing Rule "); webClient.print(editId+1); webClient.println("</b><br><br>");
            
            webClient.print("IF Source: <select id='srcSelect' name='src' onchange='updateUI()'>");
            printOption("none", "None", smartHomeRules[editId].source);
            printOption("btn1", "Button 1", smartHomeRules[editId].source);
            printOption("btn2", "Button 2", smartHomeRules[editId].source);
            printOption("rfid", "RFID Tag UID", smartHomeRules[editId].source);
            printOption("temp", "Temperature", smartHomeRules[editId].source);
            printOption("hum", "Humidity", smartHomeRules[editId].source);
            printOption("touch", "Touch Sensor", smartHomeRules[editId].source);
            printOption("motion", "Motion Sensor", smartHomeRules[editId].source);
            printOption("gas", "Gas Sensor", smartHomeRules[editId].source);
            printOption("water", "Water Level", smartHomeRules[editId].source);
            printOption("soil", "Soil Humidity", smartHomeRules[editId].source);
            webClient.println("</select><br>");
            
            webClient.print("<span id='opContainer'>Operator: <select id='opSelect' name='op'>");
            printOption("==", "Is (==)", smartHomeRules[editId].op);
            printOption(">", "Greater (>)", smartHomeRules[editId].op);
            printOption("<", "Less (<)", smartHomeRules[editId].op);
            webClient.println("</select><br></span>");

            String curVal = smartHomeRules[editId].value;
            webClient.print("Value: ");
            
            webClient.print("<input type='text' id='valInput' name='val' value='"); 
            webClient.print(curVal);
            webClient.print("' placeholder='...'>");

            webClient.print("<a href='/pairrfid?id=");
            webClient.print(editId);
            webClient.println("' id='pairBtn' style='display:none; background:#2196F3; color:white; padding:5px 10px; text-decoration:none; border-radius:4px; margin-left:10px; font-size:16px;'>Pair FOB</a>");

            webClient.print("<select id='valSelBtn' name='val' style='display:none;'>");
            printOption("tap", "Single Tap", curVal);
            printOption("double", "Double Tap", curVal);
            printOption("hold", "Hold", curVal);
            printOption("raw", "Direct Press (While Holding)", curVal);
            webClient.println("</select>");

            webClient.print("<select id='valSelBool' name='val' style='display:none;'>");
            printOption("1", "True / Detected (1)", curVal);
            printOption("0", "False / Clear (0)", curVal);
            webClient.println("</select><br>");

            webClient.print("THEN Action: <select id='actSelect' name='a' onchange='updateActionUI()'>");
            printOption("none", "None", smartHomeRules[editId].action);
            printOption("fan_toggle", "Toggle Fan", smartHomeRules[editId].action);
            printOption("fan_on", "Turn Fan ON", smartHomeRules[editId].action);
            printOption("fan_off", "Turn Fan OFF", smartHomeRules[editId].action);
            printOption("led_toggle", "Toggle LED", smartHomeRules[editId].action);
            printOption("door_toggle", "Toggle Door", smartHomeRules[editId].action);
            printOption("door_open", "Open Door", smartHomeRules[editId].action);
            printOption("door_close", "Close Door", smartHomeRules[editId].action);
            printOption("window_toggle", "Toggle Window", smartHomeRules[editId].action);
            printOption("window_open", "Open Window", smartHomeRules[editId].action);
            printOption("window_close", "Close Window", smartHomeRules[editId].action);
            printOption("buzzer_beep", "Beep Buzzer", smartHomeRules[editId].action);
            printOption("strip_toggle", "Toggle Custom LED Strip", smartHomeRules[editId].action);
            printOption("strip_momentary", "Custom LED Strip (Momentary/While Active)", smartHomeRules[editId].action);
            printOption("strip_flash", "Flash Custom LED Strip", smartHomeRules[editId].action);
            webClient.println("</select><br>");

            // Dynamic LED Config Block
            webClient.println("<div id='ledConfig' style='display:none; margin-top:10px; padding:10px; background:#ddd; border-radius:5px;'>");
            webClient.println("<b>Custom LED Strip Colors (0-255)</b><br>");
            
            int c[12] = {0};
            String curExt = smartHomeRules[editId].extra;
            if (curExt.length() > 0) {
                int startIdx = 0;
                for(int i=0; i<12; i++) {
                    int commaIdx = curExt.indexOf(',', startIdx);
                    if (commaIdx == -1) commaIdx = curExt.length();
                    c[i] = curExt.substring(startIdx, commaIdx).toInt();
                    startIdx = commaIdx + 1;
                }
            }
            
            for(int i=1; i<=4; i++) {
               webClient.print("LED "); webClient.print(i); webClient.print(": ");
               webClient.print("R <input type='number' name='r"); webClient.print(i); webClient.print("' value='"); webClient.print(c[(i-1)*3]); webClient.print("' min='0' max='255' style='width:60px;'> ");
               webClient.print("G <input type='number' name='g"); webClient.print(i); webClient.print("' value='"); webClient.print(c[(i-1)*3+1]); webClient.print("' min='0' max='255' style='width:60px;'> ");
               webClient.print("B <input type='number' name='b"); webClient.print(i); webClient.print("' value='"); webClient.print(c[(i-1)*3+2]); webClient.print("' min='0' max='255' style='width:60px;'><br>");
            }
            webClient.println("</div><br>");

            webClient.println("<button type='submit' name='act' value='save' style='background:#4CAF50; color:white; border:none;'>Save Rule</button> ");
            webClient.println("<button type='submit' name='act' value='del' style='background:#f44336; color:white; border:none;'>Delete Rule</button>");
            webClient.println("</form>");
            
            webClient.println("<br><form method='get' action='/'><input type='submit' value='Back to Network Config'></form>");
            webClient.println("</body></html>");
          } 
          else if (readString.indexOf("GET /pairrfid") >= 0) {
            int idx = getParam("id", readString).toInt();
            if (idx < 0) idx = 0;
            if (idx >= MAX_RULES) idx = MAX_RULES - 1;

            mylcd.clear();
            mylcd.setCursor(0, 0); mylcd.print("TAP FOB NOW");
            mylcd.setCursor(0, 1); mylcd.print("Waiting 10s...");
            
            String scannedUID = "";
            unsigned long startWait = millis();
            
            while(millis() - startWait < 10000) {
#ifdef RFID
              scannedUID = rfid();
              if(scannedUID != "") break;
#endif
              delay(50);
            }
            
            if(scannedUID != "") {
              mylcd.clear();
              mylcd.setCursor(0, 0); mylcd.print("SUCCESS!");
              mylcd.setCursor(0, 1); mylcd.print(scannedUID);
              smartHomeRules[idx].value = scannedUID; 
              delay(1500);
            } else {
              mylcd.clear();
              mylcd.setCursor(0, 0); mylcd.print("TIMEOUT");
              delay(1500);
            }

            webClient.println("HTTP/1.1 302 Found");
            webClient.print("Location: /rules?id=");
            webClient.println(idx);
            webClient.println("Connection: close");
            webClient.println();
          }
          else if (readString.indexOf("GET /saverule") >= 0) {
            int idx = getParam("id", readString).toInt();
            String act = getParam("act", readString);
            
            if (idx >= 0 && idx < MAX_RULES) {
              if (act == "del") {
                smartHomeRules[idx].source = "none";
                smartHomeRules[idx].op = "==";
                smartHomeRules[idx].value = "";
                smartHomeRules[idx].action = "none";
                smartHomeRules[idx].extra = "";
              } else {
                String s = getParam("src", readString);
                String o = getParam("op", readString);
                String v = getParam("val", readString);
                String a = getParam("a", readString);
                
                if (s == "btn1" || s == "btn2" || s == "rfid") {
                  o = "=="; 
                } 
                else if (s == "motion" || s == "gas") {
                  o = "=="; 
                  if (v != "0" && v != "1") v = "1"; 
                } 
                else if (s != "none") {
                  float checkNum = v.toFloat();
                  if (checkNum == 0.0 && v != "0" && v != "0.0") v = "0"; 
                }

                if (a.indexOf("strip_") == 0) {
                  String ext = "";
                  auto clamp = [](String val) {
                    if (val == "") return 0;
                    int v = val.toInt();
                    if (v < 0) return 0;
                    if (v > 255) return 255;
                    return v;
                  };
                  for (int j = 1; j <= 4; j++) {
                    ext += String(clamp(getParam("r" + String(j), readString))) + ",";
                    ext += String(clamp(getParam("g" + String(j), readString))) + ",";
                    ext += String(clamp(getParam("b" + String(j), readString))) + (j == 4 ? "" : ",");
                  }
                  smartHomeRules[idx].extra = ext;
                } else {
                  smartHomeRules[idx].extra = "";
                }

                smartHomeRules[idx].source = s;
                smartHomeRules[idx].op = o;
                smartHomeRules[idx].value = v;
                smartHomeRules[idx].action = a;
              }
              
              preferences.putString(("src" + String(idx)).c_str(), smartHomeRules[idx].source);
              preferences.putString(("op" + String(idx)).c_str(), smartHomeRules[idx].op);
              preferences.putString(("val" + String(idx)).c_str(), smartHomeRules[idx].value);
              preferences.putString(("a" + String(idx)).c_str(), smartHomeRules[idx].action);
              preferences.putString(("ext" + String(idx)).c_str(), smartHomeRules[idx].extra);
            }
            
            webClient.println("HTTP/1.1 302 Found");
            webClient.print("Location: /rules?id=");
            webClient.println(idx);
            webClient.println("Connection: close");
            webClient.println();
          } 
          else if (readString.indexOf("GET /exit") >= 0) {
             webClient.println("HTTP/1.1 200 OK");
             webClient.println("Content-Type: text/html");
             webClient.println("Connection: close");
             webClient.println();
             
             webClient.println("<html><body><h2>Rebooting...</h2></body></html>");
             delay(1000); ESP.restart();
          }
          else {
            int storedNeighborhood = preferences.getInt("neighborhood", 1);
            int storedHome = preferences.getInt("home", 1);
            String storedMQTT = preferences.getString("mqtt_ip", mqtt_server);

            webClient.println("HTTP/1.1 200 OK");
            webClient.println("Content-Type: text/html");
            webClient.println("Connection: close");
            webClient.println();

            webClient.println("<html><head><title>Config</title><style>body{font-size:22px; font-family:Arial;} input,button{font-size:22px; padding:5px; margin:5px 0;} h2{font-size:28px;}</style></head><body>");
            webClient.println("<h2>Smart Home Network Configuration</h2>");
            webClient.println("<form method='get' action='/'>");
            webClient.println("Neighborhood (1-100): <input type='number' name='neighborhood' value='" + String(storedNeighborhood) + "' min='1' max='100'><br>");
            webClient.println("Home Number (1-100): <input type='number' name='home' value='" + String(storedHome) + "' min='1' max='100'><br>");
            webClient.println("MQTT IP: <input type='text' name='mqttip' value='" + String(storedMQTT) + "'><br>");
            webClient.println("WiFi SSID: <input type='text' name='wifissid' value='" + String(ssid_config) + "'><br>");
            webClient.println("WiFi Pass: <input type='text' name='wifipass' value='" + String(pass_config) + "'><br><br>");
            webClient.println("<input type='submit' name='reset' value='Finalize and Reset ESP32' style='background:#4CAF50; color:white; border:none; cursor:pointer;'>");
            webClient.println("</form><br>");
            webClient.println("<form method='get' action='/'><input type='submit' name='default' value='Reset to Defaults'></form><br><hr><br>");
            webClient.println("<a href='/rules'><button type='button'>Go to Sandbox / Rules Config</button></a>");
            webClient.println("</body></html>");
          }
          break; 
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    } else {
      delay(1); 
    }
  }
  webClient.stop();
  
  if (readString.indexOf("default=Reset+to+Defaults") > 0) {
    preferences.putBool("reset", false); reset = false; return;  
  }
  if (readString.indexOf("reset=Finalize+and+Reset") > 0) {
    int nh = getParam("neighborhood", readString).toInt();
    int hm = getParam("home", readString).toInt();
    if(nh < 1) nh = 1; if(nh > 100) nh = 100;
    if(hm < 1) hm = 1; if(hm > 100) hm = 100;

    preferences.putBool("reset", true);
    preferences.putInt("neighborhood", nh);
    preferences.putInt("home", hm);
    preferences.putString("mqtt_ip", getParam("mqttip", readString));
    preferences.putString("wifi_ssid", getParam("wifissid", readString));
    preferences.putString("wifi_pass", getParam("wifipass", readString));
    delay(300); ESP.restart();
  }
}

// WIP COMM HERE
bool connectWiFi(int timeoutSec) {
  WiFi.begin(wifi_ssid, wifi_pass);
  int attempts = timeoutSec * 2; 
  while (WiFi.status() != WL_CONNECTED && attempts > 0) { delay(500); attempts--; }
  return WiFi.status() == WL_CONNECTED;
}

void handleWiFiFailure() {
#if defined(pushbutton1Pin) && defined(pushbutton2Pin)
  while (true) {
    mylcd.clear();
    mylcd.setCursor(0, 0); mylcd.print("1:Offln 2:Retry");
    mylcd.setCursor(0, 1); mylcd.print("Choose option...");

    while (true) {
      if (digitalRead(pushbutton1Pin) == LOW) {
        delay(200); 
        offlineMode = true;
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        mylcd.clear();
        mylcd.setCursor(0, 0); mylcd.print("Offline Mode");
        mylcd.setCursor(0, 1); mylcd.print("Rules Only");
        delay(2000);
        return;
      }
      if (digitalRead(pushbutton2Pin) == LOW) {
        delay(200); 
        mylcd.clear();
        mylcd.setCursor(0, 0); mylcd.print("Retrying 30s...");
        if (connectWiFi(30)) return; 
        break; 
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
    else if (messageTemp.toInt() == 1) {
      for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(255, 0, 0));
      strip.show();
    }
    else if (messageTemp.toInt() == 4) {
      for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 255, 0));
      strip.show();
    }
    else if (messageTemp.toInt() == 6) {
      for (int i = 0; i < strip.numPixels(); i++) strip.setPixelColor(i, strip.Color(0, 0, 255));
      strip.show();
    }
#endif
  }
  if (String(topic) == control5) {
#ifdef fanPin1    
    analogWrite(fanPin1, messageTemp.toFloat() ? ((messageTemp.toFloat()) * 130 + 125) : 0);
    digitalWrite(fanPin2, LOW); fanIsOn = (messageTemp.toFloat() > 0);
#endif    
  }
#endif
}

#ifdef LEDStripPin
void colorWipe(uint32_t color, int wait) {
  for (int i = 0; i < strip.numPixels(); i++) { strip.setPixelColor(i, color); strip.show(); delay(wait); }
}
void rainbow(int wait) {}
#endif

#ifdef dht11PIN
void temperature_humidity() {
  if (xht.receive(dht)) {                  
    lastTemp = dht[2] + dht[3] / 10.0; lastHum = dht[0] + dht[1] / 10.0;  
    if (!offlineMode) {
      client.publish(client_temperature, String(lastTemp).c_str());
      client.publish(client_humidity, String(lastHum).c_str());
    }
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
  rfidMsg.toUpperCase(); 
  if (!offlineMode && !isConfigMode) client.publish(client_rfid, (char*)rfidMsg.c_str()); 
  return rfidMsg;
}
#endif
