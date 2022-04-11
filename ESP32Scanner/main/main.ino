#include "Arduino.h"
#include "CustomWiFiConnection.h"
#include "BluetoothScanner.h"
#include "LEDBlinker.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

//Initialize services as pointers and instantiate them after the safety delay
static CustomWiFiConnection* wifiConnection;
static BluetoothScanner* bluetoothScanner;
LEDBlinker ledBlinker(4);

void setup(){
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);
  Serial.println("Started");
  for (int i = 0; i < 6; i++){ //Safety delay
    ledBlinker.invertState();
    delay(500);
  }
  Serial.println("WiFi init");
  wifiConnection = new CustomWiFiConnection();
  Serial.println("BT init");
  bluetoothScanner = new BluetoothScanner(5);
}

void loop(){
    Serial.println("WiFi keep-alive");
    wifiConnection->keepAlive();
    Serial.println("BT keep-alive");
    bluetoothScanner->keepAlive();
    ledBlinker.invertState();
    delay(500);
}
