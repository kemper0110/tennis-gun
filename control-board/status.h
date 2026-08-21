#ifndef STATUS_H
#define STATUS_H

#include <Arduino.h>

enum class ControlError : uint8_t {
  None,
  InvalidJson,
  InvalidCommand,
  InvalidValue,
};

struct DeviceState {
  bool running = false;
  uint8_t topSpeed = 0;
  uint8_t bottomSpeed = 0;
  uint8_t deliverySpeed = 0;
  ControlError error = ControlError::None;
};

const char* controlErrorAsString(ControlError error);

#endif
