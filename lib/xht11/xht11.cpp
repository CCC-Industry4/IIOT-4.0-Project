/**
 * 名字：xht11.cpp
 * 功能：xht11控制源文件
 * 适用于arduino, esp32, raspberry pi pico
 * 作者：Dengfei
 * 时间：2022.1.5
 */

#include "xht11.h"
#include "Arduino.h"

xht11::xht11(int pin)
{
  XHT11_pin = pin;
}

//发送起始信号
void xht11::start()
{
  pinMode(XHT11_pin, OUTPUT);
  digitalWrite(XHT11_pin, HIGH);
  delayMicroseconds(2);
  digitalWrite(XHT11_pin, LOW);
  delay(18);   //延时18ms以上
  digitalWrite(XHT11_pin, HIGH);//释放拉高引脚
  delayMicroseconds(30);
}

//接收一个字节
unsigned char xht11::rec_byte() {
  unsigned char i, dat = 0;
  pinMode(XHT11_pin, INPUT);
  for (i = 0; i < 8; i++) { //从高到低依次接收8位数据
    while (!digitalRead(XHT11_pin));   //等待50us低电平过去
    delayMicroseconds(40);     //延时40us，如果还为高则数据为1，否则为0
    dat <<= 1;         //移位使正确接收8位数据，数据为0时直接移位
    if (digitalRead(XHT11_pin) == 1) //数据为1时，使dat加1来接收数据1
      dat |= 1;
    while (digitalRead(XHT11_pin)); //等待数据线拉低
  }
  return dat;
}

//接收40位的数据
bool xht11::receive(unsigned char *xht) {
  bool flag;
  unsigned char RH, RL, TH, TL, revise;
  start();
  pinMode(XHT11_pin, INPUT);
  delayMicroseconds(10);
  if (digitalRead(XHT11_pin) == 0) {
    while (digitalRead(XHT11_pin) == 0); //等待拉高
    delayMicroseconds(90);  //拉高后延时80us(实测这里用90us)
    RH = rec_byte();  //接收湿度高八位
    RL = rec_byte();  //接收湿度低八位
    TH = rec_byte();  //接收温度高八位
    TL = rec_byte();  //接收温度低八位
    revise = rec_byte(); //接收校正位
    //delayMicroseconds(50);    //结束
    if ((RH + RL + TH + TL) == revise) //校正
    {
      flag = true;
      xht[0] = RH;
      xht[1] = RL;
      xht[2] = TH;
      xht[3] = TL;
      //数据处理，方便显示
      /*Serial.print(xht[0]);
        Serial.print("  ");
        Serial.print(xht[1]);
        Serial.print("  ");
        Serial.print(xht[2]);
        Serial.print("  ");
        Serial.print(xht[3]);
        Serial.print("  ");
        Serial.println(revise);*/
    } else {
      flag = false;
    }
  }
  return flag;  //读取正确温湿度值返回True,否则返回False
}

