#pragma once

//Inputs
#define IN1_pin 26
#define IN2_pin 27
#define IN3_pin 14
#define IN4_pin 12
#define EXTRA_IO_pin 13
#define sensor1 A4

//Ultrasonic Sensor 
#define TRIG_pin 25
#define ECHO_pin 33

//Potentiometer
#define POTENTIOMETER_pin 33

//Input Pins for Pushbuttons and Switches
#define ON_SW_pin 35
#define AUTO_SW_pin 34
#define START_PB_pin 39
#define STOP_PB_pin 36
#define USR_PB_pin 0

//Output Pins
#define OUT0_pin 5
#define OUT1_pin 16
#define OUT2_pin 15
#define OUT3_pin 2

//Stepper Motor Pins
#define STEPPER_pin1 4
#define STEPPER_pin2 18
#define STEPPER_pin3 17
#define STEPPER_pin4 19

//Display Pins
#define OLED_SDA_pin 22  //OLED SDA of ESP32
#define OLED_SCL_pin 21  //OLED SCL of ESP32
#define OLED_RST 16      //Optional, TTGO board contains OLED_RST connected to pin 16 of ESP32
#define OLED_ADDR 0x3c

#include <vector>

SSD1306Wire display = SSD1306Wire(OLED_ADDR, OLED_SDA_pin, OLED_SCL_pin);

void setupPins(){
  pinMode(IN1_pin, INPUT_PULLUP);
  pinMode(IN2_pin, INPUT_PULLUP);
  pinMode(IN3_pin, INPUT_PULLUP);
  pinMode(IN4_pin, INPUT_PULLUP);
  pinMode(EXTRA_IO_pin, INPUT);
  pinMode(TRIG_pin, OUTPUT);
  pinMode(ECHO_pin, INPUT);
  pinMode(POTENTIOMETER_pin, INPUT);
  pinMode(ON_SW_pin, INPUT_PULLUP);
  pinMode(AUTO_SW_pin, INPUT_PULLUP);
  pinMode(START_PB_pin, INPUT);
  pinMode(STOP_PB_pin, INPUT);
  pinMode(USR_PB_pin, INPUT);

  pinMode(OUT0_pin, OUTPUT);
  pinMode(OUT1_pin, OUTPUT);
  pinMode(OUT2_pin, OUTPUT);
  pinMode(OUT3_pin, OUTPUT);
}

String readInput(){
  String output = "";
  output = digitalRead(IN1_pin);
  output += digitalRead(IN2_pin);
  output += digitalRead(IN3_pin);
  output += digitalRead(IN4_pin);
  output += digitalRead(START_PB_pin);
  output += digitalRead(STOP_PB_pin);
  output += digitalRead(ON_SW_pin);
  output += digitalRead(AUTO_SW_pin);
  output += digitalRead(USR_PB_pin);
  return output;
}

void setup_wifi() {
  delay(10);
  display.init();
  display.setFont(ArialMT_Plain_16);
  display.drawString(0, 0, String(client_name));
  display.drawString(0, 16, "Connecting to...");
  display.drawString(0, 32, String(SSID));
  display.display();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(SSID);

  //IMPORTANT 
  IPAddress staticIP(STATIC_IP[0], STATIC_IP[1], STATIC_IP[2], STATIC_IP[3]); //NOTE: CHANGE THIS FOR EVERY DEVICE!
  IPAddress gateway(192, 168, 10, 1);
  IPAddress subnet(255, 255, 255, 0);
  //IMPORTANT

  WiFi.config(staticIP, gateway, subnet);
  
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  delay(1000);
  display.init();
  display.drawString(0, 0, "Wifi Connected!");
  display.drawString(0, 16, "Local IP address:");
  display.drawString(0, 32, WiFi.localIP().toString().c_str());
  display.display();
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(1000);
}

struct Line{
  const char* text;
  OLEDDISPLAY_TEXT_ALIGNMENT alignment;
  int row, line;
};

std::vector<Line> lines;

void displayTopic(Topic& topic, int line){
  display.setColor(WHITE);
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(127, line, topic.getVal());

  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.drawString(0, line, topic.getName());
  display.display();

  lines.push_back(Line{topic.getVal(), TEXT_ALIGN_RIGHT, 127, line});
  lines.push_back(Line{topic.getName(), TEXT_ALIGN_LEFT, 0, line});
}

void clearDisplay(){
  display.setColor(BLACK);
  while(lines.size()){
    int last = lines.size() - 1;
    display.setTextAlignment(lines[last].alignment);
    display.drawString(lines[last].row, lines[last].line, lines[last].text);
    lines.pop_back();
  }
}

