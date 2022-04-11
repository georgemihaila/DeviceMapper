#ifndef BluetoothScanner_h
#define BluetoothScanner_h
#include <BLEScan.h>
#include "Service.h"

class BluetoothScanner : public IService
{
  public:
    BluetoothScanner(int scanTime);
    void keepAlive();
  private:
    BLEScan* _pBLEScan;
    int _scanTime;
};

#endif
