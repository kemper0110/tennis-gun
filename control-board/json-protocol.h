#ifndef JSON_PROTOCOL_H
#define JSON_PROTOCOL_H

#include <Arduino.h>

void handleControlJson(const uint8_t* data, size_t length);
String getStatusJson();

#endif
