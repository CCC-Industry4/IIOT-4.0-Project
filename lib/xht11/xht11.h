/**
   名字：xht11.h
   功能：实现自定义库头文件
   适用于arduino, esp32, raspberry pi pico
   作者：Dengfei
   时间：2021/1/5
*/

#ifndef _xht11_H__
#define _xht11_H__

#include "Arduino.h"

class xht11
{
  private:
    int XHT11_pin ;

  public:
    xht11(int pin);
    void start();
    unsigned char rec_byte();
    bool receive(unsigned char *xht);
};

#endif
