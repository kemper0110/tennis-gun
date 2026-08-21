#include <Arduino.h>
#include "ble-control.h"
#include "control-state.h"
#include "motor-controller.h"

void setup() {
  Serial.begin(115200);
  setupMotors();
  initializeControlState();
  setupBleControl();
}

void loop() {
  delay(1000);
}
