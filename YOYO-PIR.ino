#include <YoYoWiFiManager.h>
#include <YoYoSettings.h>
#include <AceButton.h>
using namespace ace_button;

#define DEV

YoYoWiFiManager wifiManager;
YoYoSettings *settings;

#include <SocketIoClient.h>
SocketIoClient socketIO;

#define LED_BUILTIN 2
#define LED_BUILTIN_ON HIGH
int BUTTON_BUILTIN =  0;

String myId = "";

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

uint8_t prevWifiStatus;

void setup() {
  Serial.begin(115200);

  myId = generateID();

  settings = new YoYoSettings(512); //Settings must be created here in Setup() as contains call to EEPROM.begin() which will otherwise fail
  wifiManager.init(settings, onceConnected, onYoYoCommandGET, onYoYoCommandPOST, true);

  if ((*settings)["ids"].size() < 2) {
    Serial.println("YoYo unpaired. Starting captive portal...");
    wifiManager.begin("YoYoMachines", "blinkblink", false);
  }
  else {
    Serial.print("YoYo paired.");
    serializeJson((*settings)["ids"], Serial);
    wifiManager.begin("YoYoMachines", "blinkblink", true);
  }

  setupPins();
}

void onceConnected() {
  setupSocketIOEvents();
}

void loop() {
  uint8_t wifiStatus = wifiManager.loop();

  if (wifiStatus == YY_CONNECTED) {
    socketIO.loop();
    ledHandler();
    pirHandler();
    fanHandler();
  }

  if (wifiStatus != prevWifiStatus) {
    if (wifiStatus == YY_CONNECTED_PEER_CLIENT) {
      // Post my yoyo code to the server and get its yoyo code.
      Serial.println("Sending my id to the server");
      DynamicJsonDocument jsonDoc(64);
      jsonDoc["id"] = myId;
      serializeJson(jsonDoc, Serial);
      wifiManager.POST(WiFi.gatewayIP().toString().c_str(), "/yoyo/id", jsonDoc.as<JsonVariant>());
      jsonDoc.clear();
      wifiManager.GET(WiFi.gatewayIP().toString().c_str(), "/yoyo/id", jsonDoc);
      if (jsonDoc["id"]) {
        addIdsToSettings(myId, jsonDoc["id"]);
        (*settings).save();
      }
    }
  }

  buttonBuiltIn.check();

  prevWifiStatus = wifiStatus;
}

bool onYoYoCommandGET(JsonVariant message) {
  bool success = false;

  Serial.println("onYoYoCommandGET " + message["path"].as<String>());

  if (message["path"] == "/yoyo/settings" && settings) {
    Serial.println("GET settings.");
    message["payload"].set(*settings);
    success = true;
  }
  else if (message["path"] == "/yoyo/id") {
    Serial.println("GET id");
    DynamicJsonDocument jsonDoc(64);
    jsonDoc["id"] = myId;
    message["payload"].set(jsonDoc);
    success = true;
  }

  return (success);
}

bool onYoYoCommandPOST(JsonVariant message) {
  bool success = false;

  Serial.println("onYoYoCommandPOST " + message["path"].as<String>());
  serializeJson(message, Serial);
  Serial.println();

  if (message["path"] == "/yoyo/settings" && settings) {
    Serial.println("POST settings");
    success = wifiManager.setCredentials(message["payload"]);
    if (success) {
      // Forward POST to other clients
      wifiManager.connect();
      message["broadcast"] = true;
    }
    addIdsToSettings(message["payload"]["ids"][0], message["payload"]["ids"][1]);
    (*settings).save();
  }
  else if (message["path"] == "/yoyo/id" && settings) {
    Serial.println("POST id");
    addIdsToSettings(myId, message["payload"]["id"]);
    (*settings).save();
    success = true;
  }

  return (success);
}

void addIdsToSettings(String myId, String remoteId) {
  JsonArray ids = (*settings).createNestedArray("ids");
  if (myId != "")
    ids.add(myId);
  if (remoteId != "")
    ids.add(remoteId);
  Serial.println("Saved IDs to settings.");
}

String getRemoteId() {
  String id = "";
  for (int i = 0; i < (*settings)["ids"].size(); i++) {
    if ((*settings)["ids"][i] != myId) {
      id = (*settings)["ids"][i].as<String>();
      break;
    }
  }
  return id;
}
