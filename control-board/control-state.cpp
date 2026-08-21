#include <Arduino.h>
#include "control-state.h"
#include "motor-controller.h"

namespace {
SemaphoreHandle_t stateMutex = nullptr;
DeviceState state;

class StateLock {
 public:
  StateLock() { xSemaphoreTake(stateMutex, portMAX_DELAY); }
  ~StateLock() { xSemaphoreGive(stateMutex); }
};

void applyLockedState() {
  applyMotorState(state);
}
}  // namespace

void initializeControlState() {
  stateMutex = xSemaphoreCreateMutex();
  if (stateMutex == nullptr) {
    Serial.println("Control state mutex initialization failed");
    while (true) delay(1000);
  }
}

DeviceState getControlState() {
  StateLock lock;
  return state;
}

void setControlError(ControlError error) {
  StateLock lock;
  state.error = error;
}

void setRunning(bool running) {
  StateLock lock;
  state.running = running;
  state.error = ControlError::None;
  applyLockedState();
}

void setTopSpeed(uint8_t speed) {
  StateLock lock;
  state.topSpeed = speed;
  state.error = ControlError::None;
  applyLockedState();
}

void setBottomSpeed(uint8_t speed) {
  StateLock lock;
  state.bottomSpeed = speed;
  state.error = ControlError::None;
  applyLockedState();
}

void setDeliverySpeed(uint8_t speed) {
  StateLock lock;
  state.deliverySpeed = speed;
  state.error = ControlError::None;
  applyLockedState();
}

void stopForDisconnect() {
  StateLock lock;
  state.running = false;
  state.error = ControlError::None;
  stopMotors();
}
