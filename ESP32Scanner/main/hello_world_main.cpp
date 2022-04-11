//file: main.cpp
#include "Arduino.h"

void setup(){
  Serial.begin(115200);
  while(!Serial){
    ; // wait for serial port to connect
  }
}

void loop(){
    Serial.println("loop");
    delay(1000);
}