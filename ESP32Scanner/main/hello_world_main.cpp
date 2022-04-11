#include "Arduino.h"
#include "CustomWiFiConnection/CustomWiFiConnection.h"
#include "BluetoothScanner/BluetoothScanner.h"
#include "LEDBlinker/LEDBlinker.h"

CustomWiFiConnection wifiConnection;
BluetoothScanner bluetoothScanner(5);
LEDBlinker ledBlinker(17);

void setup(){
  Serial.begin(115200);
  while(!Serial){
    ; // wait for serial port to connect
  }
}

void loop(){
    wifiConnection.keepAlive();
    bluetoothScanner.keepAlive();
    ledBlinker.invertState();
    delay(500);
}