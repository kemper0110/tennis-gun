#include <Arduino.h>
#define ARDUINOJSON_SLOT_ID_SIZE 1
#define ARDUINOJSON_STRING_LENGTH_SIZE 1
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_LONG_LONG 0
#include <ArduinoJson.h>
#include "status.h"
#include "status-as-json.h"

extern bool running;
extern Shooter shooter;
extern Delivery delivery;

String getStatusAsJson() {
  JsonDocument doc;

  doc[F("running")] = running;
  doc[F("freeHeap")] = ESP.getFreeHeap() / 1024;

  auto shooter_json = doc[F("shooter")].to<JsonObject>();
  shooter_json[F("top_speed")] = shooter.top_speed;
  shooter_json[F("bottom_speed")] = shooter.bottom_speed;
  doc[F("delivery")][F("speed")] = delivery.speed;

  String output;
  serializeJson(doc, output);
  return output;
}