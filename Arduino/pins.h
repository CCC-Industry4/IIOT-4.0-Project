#ifdef SMARTHOME
// pins for smarthome
#define LED_COUNT 4

#define RFID
#define dht11PIN        17  //Temperature and humidity sensor pin
#define motionPin 14
#define pushbutton1Pin 16
#define pushbutton2Pin 27
#define touchPin 33
#define gasPin 23
#define LEDPin 12
#define buzzerPin 25
#define windowServo 5
#define doorServo 13
#define fanPin1 19
#define fanPin2 18
#define LEDStripPin 26
#endif

#ifdef SMARTFARM
//pins for smartfarm
#define dht11PIN        17  //Temperature and humidity sensor pin
#define gasPin    35  //Steam sensor pin
#define lightPin        34  //Photoresistor pin
#define waterLevelPin   33  //Water level sensor pin
#define soilHumidityPin 32  //Soil humidity sensor pin
#define motionPin 23
//To be controlled
#define pushbutton1Pin  5 
#define relayPin        25  //Relay pin (to control water pump)
#define doorServo        26  //Servo pin
#define fanPin1         19  //Fan IN+ pin
#define fanPin2         18  //Fan IN- pin
#define buzzerPin       16  //Buzzer pin
#define LEDPin 27  //LED pin
#endif

