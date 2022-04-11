#include "LEDBlinker.h"
#include <Arduino.h>

int _pin;
bool _isOn;

LEDBlinker::LEDBlinker(int pin){
  _pin = pin;
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, HIGH);
  _isOn = true;
}

void LEDBlinker::invertState() {
  _isOn = !_isOn;
  if (_isOn){
    digitalWrite(_pin, HIGH);
  }
  else{
    digitalWrite(_pin, LOW);
  }
}