#include "Arduino.h"
#include "CustomWiFiConnection.h"
#include "BluetoothScanner.h"
#include "LEDBlinker.h"
#include "Service.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Initialize services as pointers and instantiate them after the safety delay
static CustomWiFiConnection* wifiConnection;
static BluetoothScanner* bluetoothScanner;
LEDBlinker ledBlinker(4);

IService services[2] = {
  CustomWiFiConnection(),
  BluetoothScanner(5)
};

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.println("Started");
  for (int i = 0; i < 6; i++){ //Safety delay
    ledBlinker.invertState();
    delay(500);
  }
}

void loop(){
    for (int i = 0; i < 2; i++) {
      services[i].keepAlive();
    }
    ledBlinker.invertState();
    delay(500);
}
