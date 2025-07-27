#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <ESP32Servo.h>
#define ARDUINOJSON_SLOT_ID_SIZE 1
#define ARDUINOJSON_STRING_LENGTH_SIZE 1
#define ARDUINOJSON_USE_DOUBLE 0
#define ARDUINOJSON_USE_LONG_LONG 0
#include <ArduinoJson.h>
#include "wifi-credentials.h"

const auto SHOOTER_TOP_PIN = 4;
const auto SHOOTER_BOTTOM_PIN = 5;
const auto DELIVERY_PIN = 6;
const auto DELIVERY_IN1_PIN = 3;
const auto DELIVERY_IN2_PIN = 7;

Servo shooterTopServo, shooterBottomServo;

const int led = 13;

AsyncWebServer server(80);
AsyncEventSource statusEvents("/status-events");
AsyncCorsMiddleware cors;

bool running = false;

struct {
  int top_speed = 0;
  int bottom_speed = 0;
} shooter;

struct {
  int speed = 0;
} delivery;

bool broadcastUpdates = false;

void handleStatus(AsyncWebServerRequest *request) {
  const auto statusJson = getStatusAsJson();
  request->send(200, "application/json", statusJson);
}

void handleRoot(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Nothing to see here");
}

void notFound(AsyncWebServerRequest *request) {
  digitalWrite(led, 1);
  request->send(404, "text/plain", "Not found");
  digitalWrite(led, 0);
}

void handleStart(AsyncWebServerRequest *request) {
  running = true;
  broadcastUpdates = true;
  request->send(200);
}

void handleStop(AsyncWebServerRequest *request) {
  running = false;
  broadcastUpdates = true;
  request->send(200);
}

bool validateSpeed(int speed) {
  if (speed < 0 || speed > 100)
    return false;
  return true;
}

void handleShooter(AsyncWebServerRequest *request) {
  const auto topSpeedParam = request->getParam("top_speed");
  if (topSpeedParam != nullptr) {
    const auto speed = topSpeedParam->value().toInt();
    if (validateSpeed(speed)) {
      shooter.top_speed = speed;
      broadcastUpdates = true;
    } else {
      request->send(500, "text/plain", "Top Speed parameter is not valid");
    }
  }
  const auto bottomSpeedParam = request->getParam("bottom_speed");
  if (bottomSpeedParam != nullptr) {
    const auto speed = bottomSpeedParam->value().toInt();
    if (validateSpeed(speed)) {
      shooter.bottom_speed = speed;
      broadcastUpdates = true;
    } else {
      request->send(500, "text/plain", "Bottom Speed parameter is not valid");
    }
  }
  request->send(200);
}

void handleDelivery(AsyncWebServerRequest *request) {
  const auto speedParam = request->getParam("speed");
  if (speedParam != nullptr) {
    const auto speed = speedParam->value().toInt();
    if (validateSpeed(speed)) {
      delivery.speed = speed;
      broadcastUpdates = true;
    } else {
      request->send(500, "text/plain", "Speed parameter is not valid");
    }
  }
  request->send(200);
}

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

void setup(void) {
  pinMode(led, OUTPUT);
  digitalWrite(led, 0);

  Serial.begin(115200);

  Serial.println("periphery init");

  shooterTopServo.attach(SHOOTER_TOP_PIN);
  shooterBottomServo.attach(SHOOTER_BOTTOM_PIN);

  pinMode(DELIVERY_IN1_PIN, OUTPUT);
  pinMode(DELIVERY_IN2_PIN, OUTPUT);
  pinMode(DELIVERY_PIN, OUTPUT);
  digitalWrite(DELIVERY_IN1_PIN, LOW);
  digitalWrite(DELIVERY_IN2_PIN, LOW);


  {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    Serial.println("");

    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  if (MDNS.begin("tennis-gun")) {
    Serial.println("MDNS responder started");
  }

  {
    cors.setOrigin("*");
    cors.setMethods("POST, GET, PATCH, OPTIONS, DELETE");
    cors.setMaxAge(600);

    server.addMiddleware(&cors);

    server.on("/", HTTP_ANY, handleRoot);
    server.on("/status", HTTP_GET, handleStatus);
    server.on("/start", HTTP_POST, handleStart);
    server.on("/stop", HTTP_POST, handleStop);
    server.on("/shooter", HTTP_PATCH, handleShooter);
    server.on("/delivery", HTTP_PATCH, handleDelivery);
    server.onNotFound(notFound);
    server.addHandler(&statusEvents);

    statusEvents.onConnect([](AsyncEventSourceClient *client) {
      client->send(getStatusAsJson(), nullptr, millis());
    });

    server.begin();
  }
  Serial.println("HTTP server started");
}

void loop(void) {
  if (broadcastUpdates) {
    broadcastUpdates = false;
    Serial.println("Reacting to broadcastUpdates");
    const auto status = getStatusAsJson();
    statusEvents.send(status, nullptr, millis());
    Serial.println(status);
    if (running) {
      shooterTopServo.write(map(shooter.top_speed, 0, 100, 0, 180));
      shooterBottomServo.write(map(shooter.bottom_speed, 0, 100, 0, 180));

      digitalWrite(DELIVERY_IN1_PIN, HIGH);
      digitalWrite(DELIVERY_IN2_PIN, LOW);
      analogWrite(DELIVERY_PIN, map(delivery.speed, 0, 100, 0, 255));
    } else {
      shooterTopServo.write(0);
      shooterBottomServo.write(0);

      digitalWrite(DELIVERY_IN1_PIN, LOW);
      digitalWrite(DELIVERY_IN2_PIN, LOW);
      analogWrite(DELIVERY_PIN, 0);
    }
  }
}
