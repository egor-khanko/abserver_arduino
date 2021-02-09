#include "Dump.h"
#include "GyverStepper.h"
#include "GyverTimers.h"
#include <Arduino.h>
#include <ArduinoJson.h>

//#define ARDUINOJSON_DECODE_UNICODE 0 TODO - fix
#define FIRMWARE_VERSION "0.1.2"

// global varibles
uint16_t statusReportFrequency = 1; // in Hz

// steppers
GStepper<STEPPER2WIRE> stepperHa(200 * 8 * 2 * 64.77, 4, 5);
GStepper<STEPPER2WIRE> stepperDec(200 * 8 * 2 * 64.77, 5, 6);

// global storage variables
uint32_t pingTimer;

// print/return avalile memory
int availableMemory() {
  int size = 2048;
  byte *buf;
  while ((buf = (byte *)malloc(--size)) == NULL)
    ;
  free(buf);
  Serial.println("RAM left: " + String(size));
  return size;
};

ISR(TIMER1_A) {
  DynamicJsonDocument data(128);
  data["type"] = "status";
  data["stHaDeg"] = stepperHa.getCurrentDeg();
  data["stDecDeg"] = stepperDec.getCurrentDeg();
  String serializedJson;
  serializeJson(data, serializedJson);
  Serial.println(serializedJson);
}

void setup() {
  Serial.begin(115200);
  // timers setup
  // timer1 is for status reporting
  Timer1.setFrequency(statusReportFrequency);
  Timer1.enableISR();

  stepperHa.setRunMode(KEEP_SPEED);
  stepperDec.setRunMode(KEEP_SPEED);
  stepperHa.setMaxSpeedDeg(5);
  stepperDec.setMaxSpeedDeg(5);
  // TODO enable stepper testing.
  // stepperHa.setSpeedDeg(1);
  // stepperDec.setSpeedDeg(1);
  pinMode(13, OUTPUT);
}

// blink led number of times passed as arameter
void blinkLed(int numberOfBlinks) {
  for (int i = 0; i < numberOfBlinks; i++) {
    digitalWrite(13, HIGH);
    delay(50);
    digitalWrite(13, LOW);
    delay(50);
  }
}

// rotate [to] - rotate to some point in decimal degrees
// ping - returns pong to serial port
void doAction(DynamicJsonDocument action) {
  auto actionType = action["n"];
  for (size_t i = 0; i < actionType.size(); i++) {
    actionType[i] = tolower(actionType[i]);
  }

  if (actionType == "moveto") {
    float ha = action["p"]["ha"];
    float dec = action["p"]["dec"];

    stepperHa.setRunMode(FOLLOW_POS);
    stepperDec.setRunMode(FOLLOW_POS);

    stepperHa.setTargetDeg(ha);
    stepperDec.setTargetDeg(dec);

  } else if (actionType == "stop") {
    stepperHa.setSpeed(0);
    stepperDec.setSpeed(0);
    Serial.println("stopped");

  } else if (actionType == "track") {
    stepperHa.setRunMode(KEEP_SPEED);
    stepperHa.setSpeedDeg(degPerHour(15));

  } else if (actionType == "ping") {
    StaticJsonDocument<64> pingResponse;
    pingResponse["type"] = "pong";
    pingResponse["version"] = String(FIRMWARE_VERSION);
    String pingResponseSerialized;
    serializeJson(pingResponse, pingResponseSerialized);
    Serial.println(pingResponseSerialized);

  } else if (actionType == "test") {
    stepperHa.setRunMode(KEEP_SPEED);
    stepperDec.setRunMode(KEEP_SPEED);
    stepperHa.setSpeedDeg(1);
    stepperDec.setSpeedDeg(1);
  }
}

void loop() {
  if (Serial.available()) { // TODO change reading method
    String recievedString = Serial.readStringUntil('\n');
    StaticJsonDocument<128> recievedJson;
    deserializeJson(recievedJson, recievedString);
    doAction(recievedJson);
  }
  stepperHa.tick();
  stepperDec.tick();
  // Serial.println(stepper1.getCurrentDeg());
}