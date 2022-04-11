#include "BluetoothScanner.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <Arduino.h>

BLEScan* __pBLEScan;
int _scanTime;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        Serial.println(advertisedDevice.toString().c_str());
    }
};

BluetoothScanner::BluetoothScanner(int scanTime){
    _scanTime = scanTime;
    BLEDevice::init("");
    _pBLEScan = BLEDevice::getScan(); //create new scan
    _pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    _pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
    _pBLEScan->setInterval(100);
    _pBLEScan->setWindow(99);  // less or equal setInterval value
}

void BluetoothScanner::keepAlive(){
    BLEScanResults foundDevices = __pBLEScan->start(_scanTime, false);
    __pBLEScan->clearResults();
}