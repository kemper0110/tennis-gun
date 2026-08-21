#include <ArduinoJson.h>
#include "control-state.h"
#include "json-protocol.h"

namespace {
constexpr size_t MAX_CONTROL_LENGTH = 128;

bool hasExactFields(JsonObjectConst object, size_t count, bool requiresValue) {
  if (object.size() != count || !object["type"].is<const char*>()) return false;
  return !requiresValue || object.containsKey("value");
}

bool readSpeed(JsonObjectConst object, uint8_t& speed) {
  const JsonVariantConst value = object["value"];
  if (!value.is<int>()) return false;
  const int parsed = value.as<int>();
  if (parsed < 0 || parsed > 100) return false;
  speed = static_cast<uint8_t>(parsed);
  return true;
}

void handleSpeedCommand(JsonObjectConst object, void (*setter)(uint8_t)) {
  if (!hasExactFields(object, 2, true)) {
    setControlError(ControlError::InvalidCommand);
    return;
  }
  uint8_t speed = 0;
  if (!readSpeed(object, speed)) {
    setControlError(ControlError::InvalidValue);
    return;
  }
  setter(speed);
}
}  // namespace

void handleControlJson(const uint8_t* data, size_t length) {
  if (length == 0 || length > MAX_CONTROL_LENGTH) {
    setControlError(ControlError::InvalidJson);
    return;
  }

  JsonDocument document;
  const DeserializationError parseError = deserializeJson(document, data, length);
  if (parseError || !document.is<JsonObject>()) {
    setControlError(ControlError::InvalidJson);
    return;
  }

  const JsonObjectConst object = document.as<JsonObjectConst>();
  if (!object["type"].is<const char*>()) {
    setControlError(ControlError::InvalidCommand);
    return;
  }

  const char* type = object["type"].as<const char*>();
  if (strcmp(type, "start") == 0) {
    if (!hasExactFields(object, 1, false)) {
      setControlError(ControlError::InvalidCommand);
      return;
    }
    setRunning(true);
  } else if (strcmp(type, "stop") == 0) {
    if (!hasExactFields(object, 1, false)) {
      setControlError(ControlError::InvalidCommand);
      return;
    }
    setRunning(false);
  } else if (strcmp(type, "set_top_speed") == 0) {
    handleSpeedCommand(object, setTopSpeed);
  } else if (strcmp(type, "set_bottom_speed") == 0) {
    handleSpeedCommand(object, setBottomSpeed);
  } else if (strcmp(type, "set_delivery_speed") == 0) {
    handleSpeedCommand(object, setDeliverySpeed);
  } else {
    setControlError(ControlError::InvalidCommand);
  }
}

String getStatusJson() {
  const DeviceState state = getControlState();
  JsonDocument document;
  document["running"] = state.running;
  document["top"] = state.topSpeed;
  document["bottom"] = state.bottomSpeed;
  document["delivery"] = state.deliverySpeed;
  document["freeHeap"] = ESP.getFreeHeap() / 1024;

  const char* error = controlErrorAsString(state.error);
  if (error == nullptr) document["error"] = nullptr;
  else document["error"] = error;

  String output;
  serializeJson(document, output);
  return output;
}
