void setupPins() {
  pinMode(LED_BUILTIN, OUTPUT);

  pinMode(BUTTON_BUILTIN, INPUT);

  pinMode(FAN_PIN, OUTPUT);
  pinMode(PIR_PIN, INPUT);

  pinMode(SENS_3, INPUT_PULLUP);
  pinMode(SENS_1, INPUT_PULLUP);

  ButtonConfig* buttonConfigBuiltIn = buttonBuiltIn.getButtonConfig();
  buttonConfigBuiltIn->setEventHandler(handleButtonEvent);
  buttonConfigBuiltIn->setFeature(ButtonConfig::kFeatureClick);
  buttonConfigBuiltIn->setFeature(ButtonConfig::kFeatureLongPress);
  buttonConfigBuiltIn->setLongPressDelay(LONG_TOUCH);
}

//internal led functions

void blinkDevice() {
  if (readyToBlink == false) {
    readyToBlink = true;
  }
}

void ledHandler() {
  if (readyToBlink == true && isBlinking == false) {
    isBlinking = true;
    blinkTime = millis();
    digitalWrite(LED_BUILTIN, 1);
  }
  if (millis() - blinkTime > blinkDuration && isBlinking == true) {
    digitalWrite(LED_BUILTIN, 0);
    isBlinking = false;
    readyToBlink = false;
  }
}

void blinkOnConnect() {
  byte NUM_BLINKS = 3;
  for (byte i = 0; i < NUM_BLINKS; i++) {
    digitalWrite(LED_BUILTIN, 1);
    delay(100);
    digitalWrite(LED_BUILTIN, 0);
    delay(400);
  }
  delay(600);
}

// button functions
void handleButtonEvent(AceButton* button, uint8_t eventType, uint8_t buttonState) {
  Serial.print("ID IS: ");
  Serial.println(button->getPin());
  switch (button->getPin()) {
    case 0:
      switch (eventType) {
        case AceButton::kEventPressed:
          break;
        case AceButton::kEventReleased:
          socketIO_sendButtonPress();
          break;
        case AceButton::kEventLongPressed:
#ifdef DEV
          factoryReset();
#endif
          break;
        case AceButton::kEventRepeatPressed:
          break;
      }
      break;
  }
}

String generateID() {
  //https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint64_t chipID = ESP.getEfuseMac();
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  return  out;
}

String generateID(uint64_t chipID) {
  //https://github.com/espressif/arduino-esp32/issues/3859#issuecomment-689171490
  uint32_t low = chipID % 0xFFFFFFFF;
  uint32_t high = (chipID >> 32) % 0xFFFFFFFF;
  String out = String(low);
  return  out;
}

long checkSensLength() {
  if (digitalRead(SENS_3) == 0 && digitalRead(SENS_1) == 1) {
    Serial.println("On time is 3 seconds");
    return 3000;
  } else if (digitalRead(SENS_1) == 0 && digitalRead(SENS_3) == 1) {
    Serial.println("On time is 30 seconds");
    return 30000;
  } else if (digitalRead(SENS_3) == 0 && digitalRead(SENS_1) == 0) {
    Serial.println("On time is 1 seconds");
    return 1000;
  } else {
    Serial.println("On time is 10 seconds");
    return 10000;
  }
}
