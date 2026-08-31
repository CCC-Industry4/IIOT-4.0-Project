/*
   xht11子程序,适用于arduino, esp32, raspberry pi pico
   2022/1/5
   Dengfei
   示例代码只读取温湿度整数部分 
*/
#include "xht11.h"
//gpio22
xht11 xht(22);

unsigned char dht[4] = {0, 0, 0, 0};//这里只接收前32位数据,未接收校验位
void setup() {
  Serial.begin(9600);//启动串口监视器,并设置波特率为9600

}

void loop() {
  if (xht.receive(dht)) { //检验正确返回true
    Serial.print("RH:");
    Serial.print(dht[0]); //湿度的整数部分,dht[1]为小数部分
    Serial.print("%  ");
    Serial.print("Temp:");
    Serial.print(dht[2]); //温度的整数部分,dht[3]为小数部分
    Serial.println("C");
  } else {    //读取错误
    Serial.println("sensor error");
  }
  delay(1500);  //需要1500ms的时间来等待设备读取

}
