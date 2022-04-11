#ifndef CustomWiFiConnection_h
#define CustomWiFiConnection_h
#include <WiFiMulti.h>
#include "Service.h"

class CustomWiFiConnection : public IService
{
  public:
    CustomWiFiConnection();
    void keepAlive();
  private:
    WiFiMulti _wifiMulti;
};

#endif
