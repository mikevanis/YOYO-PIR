#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>
#include <AceButton.h>
using namespace ace_button;

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

#include <SocketIoClient.h>
SocketIoClient socketIO;

#define LED_BUILTIN 2
#define LED_BUILTIN_ON HIGH
int BUTTON_BUILTIN =  0;

String myID = "";
String remoteID = "";

//PIR variables
int PIR_sensitivity;
#define PIR_PIN 14
#define FAN_PIN 13
#define SENS_3 26
#define SENS_1 25
#define FANRUNTIME 10000
int fan_on_time = 10;
long currFanRunTime;
bool isFanRunning = false;
long prevPirCheckMillis;
#define PIRCHECKINTERVAL 50
bool hasSentPIR = false;
#define PIRSENDINTERVAL 3000
long prevPirOnMillis = 0;

/// Led Settings ///
bool isBlinking = false;
bool readyToBlink = false;
unsigned long blinkTime;
int blinkDuration = 200;

//Button Settings
AceButton buttonBuiltIn(BUTTON_BUILTIN);
#define LONG_TOUCH 1500

/// Socket.IO Settings ///
#ifndef STAGING
char host[] = "irs-socket-server.herokuapp.com"; // Socket.IO Server Address
#else
char host[] = "irs-socket-server-staging.herokuapp.com"; // Socket.IO Staging Server Address
#endif
int port = 80; // Socket.IO Port Address
char path[] = "/socket.io/?transport=websocket"; // Socket.IO Base Path

void setup() {
  Serial.begin(115200);

  myID = generateID();

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, onceConnected, onYoYoCommandGET, onYoYoCommandPOST, true);

  remoteID = (*settings)["remoteID"].as<String>();
  if (remoteID.length())
    wifiManager.begin("YoYoMachines", "blinkblink", true);
  else
    wifiManager.begin("YoYoMachines", "blinkblink", false);
}

void onceConnected() {
  setupSocketIOEvents();
}

void loop() {
  uint8_t wifiStatus = wifiManager.loop();

  switch(wifiStatus) {
    case YY_NO_SHIELD:
      break;
    case YY_IDLE_STATUS:
      break;
    case YY_NO_SSID_AVAIL:
      break;
    case YY_SCAN_COMPLETED:
      break;
    case YY_CONNECTED:
      socketIO.loop();
      ledHandler();
      pirHandler();
      fanHandler();
      buttonBuiltIn.check();
      break;
    case YY_CONNECT_FAILED:
      break;
    case YY_CONNECTION_LOST:
      break;
    case YY_DISCONNECTED:
      break;
    case YY_CONNECTED_PEER_CLIENT:
      break;
    case YY_CONNECTED_PEER_SERVER:
      break;
  }
}

bool onYoYoCommandGET(const String &url, JsonVariant json) {
  bool success = false;

  Serial.println("onYoYoCommandGET " + url);
  
  if(url.equals("/yoyo/settings") && settings) {
    success = true;
    json.set(*settings);
  }

  return(success);
}

bool onYoYoCommandPOST(const String &url, JsonVariant json) {
  bool success = false;
  
  Serial.println("onYoYoCommandPOST " + url);
  serializeJson(json, Serial);

  //define an alternative to using the built-in /yoyo/credentials endpoint that also allows custom value to be set in the settings doc
  if(url.equals("/yoyo/settings")) {
    (*settings)["remoteID"] = json["remoteID"];
    
    success = wifiManager.setCredentials(json);
    if(success) {
      wifiManager.broadcastToPeersPOST(url, json);
      wifiManager.connect();
    }
    
    (*settings).save();
  }

  return(success);
}
