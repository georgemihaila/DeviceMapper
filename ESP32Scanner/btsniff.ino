#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64 
#define OLED_RESET     -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET); //at 0x3C

//#include "Ucglib.h"

#define ESP_AUTO_RESTART_TIME_MS 600000 //Restart every 10 minutes just in case
#define EXTERNAL_LED 13
#define TIME_BEFORE_FORCE_CACHE 1000

#include "TinyGPSPlus.h"
TinyGPSPlus gps;

bool gpsHeardFrom = false;
int nSatellites = 0;
String longitude;
String latitude;
String altitude;

#define VOLTAGE_DIVIDER_CORRECTION (3.71 / 3.68)
#define VOLTAGE_DIVIDER_RATIO (3.69 / 1.86) * VOLTAGE_DIVIDER_CORRECTION

double getSourceVoltage(){
  return analogReadMilliVolts(A4) * VOLTAGE_DIVIDER_RATIO / 1000;
}

String generateLocationCSV(){
  return longitude + "," + latitude + "," + altitude;
}

bool hasSD = false;
void appendFile(fs::FS &fs, const char * path, const char * message){
    if (!hasSD){
      return;
    }
    int errorCount = 0;
    bool ok = false;
    do{
        File file = fs.open(path, FILE_APPEND);
        if(!file){
            Serial.println("Failed to open file for appending");
            errorCount++;
            return;
        }
        else{
            if(file.println(message)){
                errorCount = 0;
                ok = true;
            } 
            else {
                errorCount++;
                Serial.println("Write error");
            }
        }
        file.close();
    }
    while (!ok && errorCount < 3);
    if (errorCount == 3){
      Serial.println("Too many SD card errors");
      ESP.restart();
    }
}

void appendCollectionLogFile(String dataType, String data){
  appendFile(SD, String("/" + dataType + ".log").c_str(), data.c_str());
}

//How many unique devices to store before sending data to server
#define COLLECT_UNIQUE_SIZE 50

//If the queue doesn't fill up in this amount of time (ms), send the data anyway
#define BLUETOOTH_SEND_DATA_EVERY_MS 10000
long lastDataSentMs = 0;
int lastSendIndex = 0;

String collectedBluetoothDevices[COLLECT_UNIQUE_SIZE];
int collectedBluetoothDeviceCount = 0;

void sendBluetoothData(){
  sendQueueData(collectedBluetoothDevices, collectedBluetoothDeviceCount, lastSendIndex, lastDataSentMs, "Bluetooth/ProcessRawString", "Bluetooth", TIME_BEFORE_FORCE_CACHE);
}

#define GPS_UPDATE_MS 1000
long lastGPSUpdateMs = 0;

#define WIFI_SEND_DATA_EVERY_MS 10000
long lastWiFiDataSentMs = 0;
int lastWiFiSendIndex = 0;

String collectedWiFiDevices[COLLECT_UNIQUE_SIZE];
int collectedWiFiDeviceCount = 0;

void sendWiFiData(){
  sendQueueData(collectedWiFiDevices, collectedWiFiDeviceCount, lastWiFiSendIndex, lastWiFiDataSentMs, "WiFi/ProcessRawString", "WiFi", TIME_BEFORE_FORCE_CACHE);
}

long lastSendAnyMs = 0;
#define WIFI_NO_SEND_DISCONNECT_TIME_MS 30000

String serverName = "http://10.10.0.241:6488/";
WiFiClient client;
HTTPClient http;
bool offlineMode = true;
bool initWiFi(int maxReattempts = 3) {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int totalAttempts = 0;
  while (WiFi.status() != WL_CONNECTED){
    setCurrentAction("Scanning for known networks...");
    int n = WiFi.scanNetworks();
    String connectTo = "";
    String password = "";
    for (int i = 0; i < n; ++i) {
      if (WiFi.SSID(i).equals("")){
        connectTo = "";
        password = "";
        break;
      }
    }
    if (!connectTo.equals("")){
      Serial.println("Connecting to " + connectTo);
      WiFi.begin(connectTo.c_str(), password.c_str());
      int c = 0;
      while (WiFi.status() != WL_CONNECTED) {
        Serial.print('.');
        delay(500);
        c++;
        if (c >= 5 * 1000 / 500){
          setCurrentAction("\nCouldn't connect");
          totalAttempts++;
          break;
        }
      }
      Serial.println();
    }
    else {
      totalAttempts++;
    }
    if (totalAttempts > maxReattempts){
      break;
    }
  }
  if (totalAttempts > maxReattempts){
    offlineMode = true;
    Serial.println("Failed to connect to WiFi, turned on offline mode");
    return false;
  }
  Serial.println(WiFi.localIP());
  return true;
}

bool makeSureWiFiConnectionUp(){
  if (offlineMode) return false;
  
  if (WiFi.status() != WL_CONNECTED){
    return initWiFi(1);
  }
  return true;
}

void deleteFile(fs::FS &fs, const char * path){
  Serial.printf("Deleting file: %s\n", path);
  if(fs.remove(path)){
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
char getRandomChar(){
   byte randomValue = random(0, 37);
   char letter = randomValue + 'a';
   if(randomValue > 26)
      letter = (randomValue - 26) + '0';
   return letter;
}

String getRandomString(int length){
  String result = "";
  for(int i = 0; i < length; i++){
    result += getRandomChar();
  }
  return result;
}

String getFailedFileName(String dataType){
  return "/failed_" + dataType + ".log";
}

void writeFailedSendToSDCard(String dataType, String data){
  String filename = getFailedFileName(dataType);
  String text = data;
  appendFile(SD, filename.c_str(), text.c_str());
}

bool postData(String data, String dataType){
    bool ok = false;
    String serverPath = serverName + dataType + "/ProcessRawString";
    http.begin(client, serverPath);
    http.addHeader("Content-Type", "application/json");
    String message = data;
    bool isJSON = data[0] == '"';
    if (!isJSON){
      message = "\"" + data + "\"";
    }
    int httpResponseCode = http.POST(message);
    //Serial.println(message);
    if (httpResponseCode == 201) {
      //All good
      Serial.println(" [OK]");
      ok = true;
    }
    else if (httpResponseCode < 0){ //ESP error?
    }
    else {
      Serial.print("HTTP response code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
    return ok;
}

void sendQueueData(String data[], int dataCount, int& lastSendIndex, long& lastDataSentMs, String path, String dataType, long maxTimeAllowedBeforeCachingMs){
  bool connected = makeSureWiFiConnectionUp();
  if (lastSendIndex > dataCount) //Queue has filled in the meantime
  {
    lastSendIndex = 0;
  }
  if (connected){
    long startMs = millis();
    bool forceCache = false;
    for (int i = lastSendIndex; i < dataCount; i++){
      if (!forceCache && maxTimeAllowedBeforeCachingMs > 0){
        if (millis() - startMs >= maxTimeAllowedBeforeCachingMs){
          forceCache = true;
          Serial.println("Force caching because requests took too long");
        }
      }
      bool ok = false;
      if (!forceCache){
        setCurrentAction(dataType + " " + String(i + 1) + "/" + String(dataCount));
        ok = postData(data[i], dataType);
      }
      if (!ok){ //Don't bother retrying now, save to SD
        setCurrentAction(dataType + ">SD " + String(i + 1) + "/" + String(dataCount));
        writeFailedSendToSDCard(dataType, data[i]);
      }
    }
  }
  else if (hasSD) {
    Serial.println("Fast WiFi not available, saving data to SD card");
    //appendFile(SD, String("/" + dataType + ".log").c_str(), data.c_str());
    for (int i = lastSendIndex; i < dataCount; i++){
      setCurrentAction(dataType + ">SD (" + String(i) + "/" + String(dataCount) + ")");
      writeFailedSendToSDCard(dataType, "\"" + data[i] + "\"");
    }
  }
  else{
    Serial.println("Data possibly lost because WiFi and SD card not available.");
  }
    lastDataSentMs = millis();
    lastSendIndex = dataCount;
    lastSendAnyMs = millis();
}

int sessionBTDevices = 0;
int sessionWiFiNetworks = 0;

bool addDeviceToListIfNewAndSendDataIfQueueFull(String (&collectedDevices)[COLLECT_UNIQUE_SIZE], int& collectedDeviceCount, int& sessionDevices, String deviceData, String deviceType, void (&sendFunction)()){
  for (int i = 0; i < collectedDeviceCount; i++){
    if (collectedDevices[i].equals(deviceData)){
      return false;
    }
  }
  appendCollectionLogFile(deviceType, deviceData);
  collectedDevices[collectedDeviceCount++] = deviceData;
  sessionDevices++;
  Serial.println("New " + deviceType + " device found. Session total: " + String(sessionDevices));
  if (collectedDeviceCount == COLLECT_UNIQUE_SIZE){
    sendFunction();
    collectedDeviceCount = 0; //Start over, keep queue for comparison
  }
  return true;
}

bool addBluetoothDeviceIfNewAndSendDataIfQueueFull(String data){
  return addDeviceToListIfNewAndSendDataIfQueueFull(collectedBluetoothDevices, collectedBluetoothDeviceCount, sessionBTDevices, data, "Bluetooth", sendBluetoothData);
}

bool addWiFiDeviceIfNewAndSendDataIfQueueFull(String data){
  return addDeviceToListIfNewAndSendDataIfQueueFull(collectedWiFiDevices, collectedWiFiDeviceCount, sessionWiFiNetworks, data, "WiFi", sendWiFiData);
}

int scanTime = 1; //In seconds
BLEScan* pBLEScan;

int bluetoothDevicesCount[128];
int currentBluetoothScanCount = 0;

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String data = generateLocationCSV() + "," + String(advertisedDevice.toString().c_str());
      if (addBluetoothDeviceIfNewAndSendDataIfQueueFull(data)){
        //currentBluetoothScanCount++;
      }
      currentBluetoothScanCount++;
    }
};

char HexLookUp[] = "0123456789abcdef";
String bssidToString(uint8_t *bssid){
  String result;
  for (int i = 0; i < 6; i++){
    if (i % 2 == 1){
      result += ":";
    }
    result += HexLookUp[bssid[i] >> 4];
    result += HexLookUp[bssid[i] & 0x0F];
  }
  return result;
}

int wifiNetworksCount[128];

void appendLastElement(int (&data)[128], int entry){
  data[127] = entry;
  for(int i = 0; i < 127; i++){
    data[i] = data[i + 1];
  } 
}

bool hasDisplay = false;
void tryInitDisplay(){
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { 
    Serial.println(F("Display not detected"));
  }
  else{
    Serial.println("Display ok");
    hasDisplay = true;
    display.display();
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.print("ready");
    display.display();
  }
}


String currentAction;

void setCurrentAction(String action){
  Serial.println(action);
  currentAction = action;
  refreshDisplay();
}

double customMap(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void graph(int data[], int count, int yOffset, bool invertVertically, int min, int max, int height){
  int graphBottom = 64 + yOffset - height;
  for(int i = 0; i < count - 1; i++){
    int h1 = (int)customMap(data[i], min, max, 0, height);
    int y1 = graphBottom - h1;
    if (invertVertically){
      y1 = graphBottom - height;
    }
    display.drawFastVLine(i, y1, h1, SSD1306_WHITE);
    //display.drawLine();
  }
}

int max(int a, int b){
  if (a >= b)
    return a;
  return b;
}

int max(int data[], int count){
  int max = data[0];
  for(int i = 0; i < count; i++){
    if (data[i] > max){
      max = data[i];
    }
  }
  return max;
}

void graph(int data[], int count, int yOffset, bool invertVertically = false, int height = 32){
  graph(data, count, yOffset, invertVertically, 0, max(data, count), height);
}

void graph(int data[], int count, int yOffset, int max, bool invertVertically = false, int height = 32){
  graph(data, count, yOffset, invertVertically, 0, max, height);
}

void dualLineGraphCentered(int data1[128], int data2[128]){
  int absoluteMax = max(max(data1, 128), max(data2, 128));
  graph(data1, 128, 0, absoluteMax); //top
  graph(data2, 128, 32, absoluteMax, true);//bottom
  //Separate charts a bit
  //display.drawFastHLine(0, 31, 128, SSD1306_BLACK);
  //display.drawFastHLine(0, 32, 128, SSD1306_BLACK);
}

void drawCentreString(const String &buf, int x, int y)
{
    int16_t x1, y1;
    uint16_t w, h;
    display.getTextBounds(buf, x, y, &x1, &y1, &w, &h); //calc width of new string
    display.setCursor(x - w / 2, y);
    display.print(buf);
}

void printAt(int x, int y, String text){
  display.setCursor(x, y);
  display.print(text);
}

int lastFPS = 0;
void refreshDisplay(){
  if (hasDisplay){
    long startMS = millis();
    
    display.setTextSize(2);
    display.setTextColor (WHITE,BLACK);
    display.clearDisplay();
    dualLineGraphCentered(bluetoothDevicesCount, wifiNetworksCount);

    display.setTextSize(1);
    printAt(5, 10, String(bluetoothDevicesCount[127]));
    printAt(5, 10 + 32, String(wifiNetworksCount[127]));    
    
    display.setTextSize(2);
    drawCentreString("B:" + String(sessionBTDevices), 64, 10);

    display.setTextSize(1);
    drawCentreString(currentAction, 64, 30);
    display.setTextSize(2);

    drawCentreString("W:" + String(sessionWiFiNetworks), 64, 10 + 32);

    String topLeftStatusString = "";
    if (offlineMode){
      topLeftStatusString += "o";
    }
    if (gpsHeardFrom){
      topLeftStatusString += "G" + String(nSatellites);
    }
    display.setTextSize(1);
    printAt(0, 0, topLeftStatusString);

    double voltage = getSourceVoltage();
    String batteryMessage = String(voltage) + "V";
    bool isLowVoltage = voltage <= 3.6;
    int x = 98;
    if (isLowVoltage){
      x = 80;
      batteryMessage = "(!)" + batteryMessage;
    }
    printAt(x, 0, batteryMessage);
    
    display.display();
    long endMS = millis();
    lastFPS = 1000 / (endMS - startMS);
  }
}

int quickBTScanTime = 1;
int deepBTScanTime = 1;
int quickBTScans = 0;

int getBTScanTime(){
  if (++quickBTScans > 5){
    quickBTScans = 0;
    return deepBTScanTime;
  }
  else{
    return quickBTScanTime;
  }
}

void BTScan(){
  int btScanTime = getBTScanTime();
  setCurrentAction("B");
  currentBluetoothScanCount = 0;
  BLEScanResults foundDevices = pBLEScan->start(btScanTime, false);
  pBLEScan->clearResults();   // delete results fromBLEScan buffer to release memory
  appendLastElement(bluetoothDevicesCount, currentBluetoothScanCount);
}

void classicWiFiScan(){
  int n = WiFi.scanNetworks();
  int newNetworks = 0;
  for (int i = 0; i < n; ++i){
    String data = String(WiFi.SSID(i));
    data += "," + String(WiFi.encryptionType(i));
    data += "," + String(WiFi.channel(i));
    data += "," + String(bssidToString(WiFi.BSSID(i)));
    data += "," + generateLocationCSV();
    if (addWiFiDeviceIfNewAndSendDataIfQueueFull(data)){
      newNetworks++;
      appendCollectionLogFile("WiFi", data);
    }
  }
  appendLastElement(wifiNetworksCount, n);
}

#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_types.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_event_loop.h"
#include "nvs_flash.h"

uint8_t level = 0, channel = 1;

static wifi_country_t wifi_country = {.cc="CN", .schan = 1, .nchan = 13}; //Most recent esp32 library struct

typedef struct {
  unsigned frame_ctrl:16;
  unsigned duration_id:16;
  uint8_t addr1[6]; /* receiver address */
  uint8_t addr2[6]; /* sender address */
  uint8_t addr3[6]; /* filtering address */
  unsigned sequence_ctrl:16;
  uint8_t addr4[6]; /* optional */
} wifi_ieee80211_mac_hdr_t;

typedef struct {
  wifi_ieee80211_mac_hdr_t hdr;
  uint8_t payload[0]; /* network data ended with 4 bytes csum (CRC32) */
} wifi_ieee80211_packet_t;

static esp_err_t event_handler(void *ctx, system_event_t *event);
static void wifi_sniffer_init(void);
static void wifi_sniffer_set_channel(uint8_t channel);
static const char *wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type);
//static void wifi_sniffer_packet_handler(void *buff, wifi_promiscuous_pkt_type_t type);

esp_err_t event_handler(void *ctx, system_event_t *event)
{
  return ESP_OK;
}


String collectedWirelessDevices[50];
int collectedWirelessDevicesCount = 0;
int sessionWirelessDevices = 0;
int lastWirelessDevicesSendIndex = 0;
long lastWirelessDevicesSentMs = 0;

void sendWirelessDevicesData(){
  return;
  sendQueueData(collectedWirelessDevices, collectedWirelessDevicesCount, lastWirelessDevicesSendIndex, lastWirelessDevicesSentMs, "Wireless/ProcessRawString", "WirelessDevice", TIME_BEFORE_FORCE_CACHE);
}

bool addWirelessDeviceIfNewAndSendDataIfQueueFull(String data){
  return addDeviceToListIfNewAndSendDataIfQueueFull(collectedWirelessDevices, collectedWirelessDevicesCount, sessionWirelessDevices, data, "WirelessDevice", sendWirelessDevicesData);
}

void wifi_sniffer_packet_handler(void* buff, wifi_promiscuous_pkt_type_t type)
{
  if (type != WIFI_PKT_MGMT)
    return;

  const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;
  const wifi_ieee80211_packet_t *ipkt = (wifi_ieee80211_packet_t *)ppkt->payload;
  const wifi_ieee80211_mac_hdr_t *hdr = &ipkt->hdr;
  char buffer[18];
  sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr1[0],hdr->addr1[1],hdr->addr1[2], hdr->addr1[3],hdr->addr1[4],hdr->addr1[5]);
  String a1 = String(buffer);
  sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],hdr->addr2[3],hdr->addr2[4],hdr->addr2[5]);
  String a2 = String(buffer);
  sprintf(buffer, "%02x:%02x:%02x:%02x:%02x:%02x", hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]);
  String a3 = String(buffer);
  bool newDevice = false;
  if (addWirelessDeviceIfNewAndSendDataIfQueueFull(a1)){
    collectedWirelessDevicesCount++; newDevice = true;
  }
  if (addWirelessDeviceIfNewAndSendDataIfQueueFull(a2)){
    collectedWirelessDevicesCount++; newDevice = true;
  }
  /*
  if (addWirelessDeviceIfNewAndSendDataIfQueueFull(a3)){
    collectedWirelessDevicesCount++; newDevice = true;
  }*/
  if (newDevice){
    Serial.println("New wireless devices detected. Total: " + String(collectedWirelessDevicesCount));
  }
  /*
  printf("PACKET TYPE=%s, CHAN=%02d, RSSI=%02d,"
    " ADDR1=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR2=%02x:%02x:%02x:%02x:%02x:%02x,"
    " ADDR3=%02x:%02x:%02x:%02x:%02x:%02x\n",
    wifi_sniffer_packet_type2str(type),
    ppkt->rx_ctrl.channel,
    ppkt->rx_ctrl.rssi,

    hdr->addr1[0],hdr->addr1[1],hdr->addr1[2],
    hdr->addr1[3],hdr->addr1[4],hdr->addr1[5],

    hdr->addr2[0],hdr->addr2[1],hdr->addr2[2],
    hdr->addr2[3],hdr->addr2[4],hdr->addr2[5],

    hdr->addr3[0],hdr->addr3[1],hdr->addr3[2],
    hdr->addr3[3],hdr->addr3[4],hdr->addr3[5]
  );*/
}

void wifi_sniffer_init(void)
{
  wifi_reinit();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&wifi_sniffer_packet_handler);
}

void wifi_reinit(){
  nvs_flash_init();
  tcpip_adapter_init();
  //ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
  ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
  ESP_ERROR_CHECK( esp_wifi_set_country(&wifi_country) ); /* set country for channel range [1, 13] */
  ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
  ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_NULL) );
  ESP_ERROR_CHECK( esp_wifi_start() );
}

void wifi_sniffer_set_channel(uint8_t channel)
{
  esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}

const char * wifi_sniffer_packet_type2str(wifi_promiscuous_pkt_type_t type)
{
  switch(type) {
  case WIFI_PKT_MGMT: return "MGMT";
  case WIFI_PKT_DATA: return "DATA";
  default:  
  case WIFI_PKT_MISC: return "MISC";
  }
}

void promiscuousWiFiScan(){
  wifi_sniffer_init();
  for (int i = 1; i < 13; i++){
    channel = i;
    wifi_sniffer_set_channel(channel);
    delay(100);
  }
  wifi_reinit();
}

void WiFiScan(){
  setCurrentAction("W");
  classicWiFiScan();
}


void renameFile(fs::FS &fs, const char * path1, const char * path2){
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

String buffer;

int getNumberOfLines(String filename) {
  File file = SD.open(filename);
  if (!file) {
    Serial.println("Can't open " + filename);
    return 0;
  }
   int line = 0;
   while (file.available()) {
    line++;
    buffer = file.readStringUntil('\n');
  }
  file.close();
  return line;
}

int getNumberOfFailedDataSends(String dataType){
  return getNumberOfLines(getFailedFileName(dataType));
}

File failedDataFile;
void trySendFailedData(String dataType){
  String filename = getFailedFileName(dataType);
  failedDataFile = SD.open(filename);
  if (!failedDataFile) {
    Serial.println("Can't open " + filename);
    return;
  }
   int line = 0;
   while (failedDataFile.available()) {
    line++;
    setCurrentAction("Failed " + dataType + " (" + line + ")");
    buffer = failedDataFile.readStringUntil('\n');
    Serial.println(buffer);
    int totalTries = 0;
    while (totalTries++ < 3){
      buffer.trim();
      if (postData(buffer, dataType)){
        break;
      }
      else{
        Serial.println("Retrying " + buffer + "...");
      }
    }
    if (totalTries == 3){
      //Completely failed
      appendFile(SD, ("/completely failed " + dataType + ".log").c_str(), buffer.c_str());
    }
  }
  failedDataFile.close();
  String randomString = getRandomString(16);
  //deleteFile(SD, filename.c_str());
  renameFile(SD, filename.c_str(), (filename + "." + randomString + ".old.log").c_str());
}

void setup() {
  tryInitDisplay();
  Serial.begin(115200);
  setCurrentAction("ESP32 up");
  if (getSourceVoltage() < 3.4){
    setCurrentAction("Low voltage");
    delay(10 * 1000);
  }
  if(!SD.begin()){
        setCurrentAction("Connect SD card reader!");
        delay(2000);
        ESP.restart();
    }
  else{
    uint8_t cardType = SD.cardType();
    if(cardType == CARD_NONE){
        setCurrentAction("Connect SD card!");
        delay(2000);
        ESP.restart();
    }
    else{
      hasSD = true;
      setCurrentAction("SD ok");
      uint64_t cardSize = SD.cardSize() / (1024 * 1024);
      Serial.printf("SD Card Size: %lluMB\n", cardSize);
    }
  }
  
  BLEDevice::init("ESP32 Scanner");
  pBLEScan = BLEDevice::getScan(); //create new scan
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true); //active scan uses more power, but get results faster
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);  // less or equal setInterval value
  
  Serial2.begin(9600, SERIAL_8N1, 16, 17);
  setCurrentAction("Started GPS serial");
  if (initWiFi(0)){ //Only do this at home, the mobile hotspot is too slow
      offlineMode = false;
      trySendFailedData("Bluetooth");
      trySendFailedData("WiFi");
  }
  else{
    sessionBTDevices = getNumberOfFailedDataSends("Bluetooth");
    sessionWiFiNetworks = getNumberOfFailedDataSends("WiFi");
  }
  //promiscuousWiFiScan(); //unstable
}

void BTAutosend(){
  if (millis() - lastDataSentMs > BLUETOOTH_SEND_DATA_EVERY_MS){ //Enough time elapsed for an autosend
    if (lastSendIndex < collectedBluetoothDeviceCount){ //New Bluetooth devices added since last send
      setCurrentAction("BT autosend");
      sendBluetoothData();
    }
  }
}

void WiFiAutosend(){
  if (millis() - lastWiFiDataSentMs > WIFI_SEND_DATA_EVERY_MS){ //Enough time elapsed for an autosend
    if (lastWiFiSendIndex < collectedWiFiDeviceCount){ //New WiFi devices added since last send
      setCurrentAction("WiFi autosend");
      sendWiFiData();
    }
  }
}

void gpsDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (Serial2.available()) {
      int c = Serial2.read();
      gps.encode(c);
      Serial.print((char)c);
      if (!gpsHeardFrom){
        gpsHeardFrom = true;
      }
    }
  } while (millis() - start < ms);
}


void GPSUpdate(){
    setCurrentAction("GPS");
    gpsDelay(500);
    bool updated = false;
    if (gps.location.isUpdated()){
      latitude = String(gps.location.lat());
      longitude = String(gps.location.lng());
      updated = true;
    }
    if (gps.altitude.isUpdated()){
     altitude = String(gps.altitude.meters());
     updated = true;
    }
    nSatellites = gps.satellites.value();
    if (updated){
      Serial.println("Longitude: " + longitude + "\nLatitude: " + latitude + "\nAltitude: " + altitude);
    }
}

void AutoDisconnect(){
  if (millis() - lastSendAnyMs > WIFI_NO_SEND_DISCONNECT_TIME_MS){ //Nothing sent in this interval, disconnect WiFi
    if (WiFi.status() == WL_CONNECTED){
      WiFi.disconnect();
      Serial.println("WiFi auto disconnected because of inactivity");
    }
  }
}

void AutoRestart(){
  if (millis() >= ESP_AUTO_RESTART_TIME_MS){
    Serial.println("Auto restart triggered");
    ESP.restart();
  }
}

void loop() {
  BTScan();
  WiFiScan();
  BTAutosend();
  WiFiAutosend();
  //GPSUpdate();
  AutoDisconnect();
  //AutoRestart();
}
