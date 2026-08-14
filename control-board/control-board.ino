#include <Arduino.h>
#include <AsyncTCP.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include "wifi-credentials.h"
#include "status-as-json.h"
#include "status.h"

constexpr uint8_t SHOOTER_TOP_PIN = 5;
constexpr uint8_t SHOOTER_BOTTOM_PIN = 4;
constexpr uint8_t DELIVERY_PIN = 6;
constexpr uint8_t DELIVERY_IN1_PIN = 7;
constexpr uint8_t DELIVERY_IN2_PIN = 3;

constexpr uint8_t SHOOTER_TOP_CHANNEL = 0;
constexpr uint8_t SHOOTER_BOTTOM_CHANNEL = 1;
constexpr uint8_t DELIVERY_CHANNEL = 2;

constexpr uint32_t SHOOTER_FREQUENCY_HZ = 50;
constexpr uint8_t SHOOTER_RESOLUTION_BITS = 14;
constexpr uint16_t SHOOTER_STOP_PULSE_US = 1000;
constexpr uint16_t SHOOTER_MIN_PULSE_US = 1130;
constexpr uint16_t SHOOTER_MAX_PULSE_US = 2000;

constexpr uint32_t DELIVERY_FREQUENCY_HZ = 20000;
constexpr uint8_t DELIVERY_RESOLUTION_BITS = 10;

AsyncWebServer server(80);
AsyncEventSource statusEvents("/status-events");
AsyncCorsMiddleware cors;

extern bool running;
extern Shooter shooter;
extern Delivery delivery;

bool broadcastUpdates = false;

uint32_t shooterPulseToDuty(uint16_t pulseUs) {
  return static_cast<uint32_t>(
      (static_cast<uint64_t>(pulseUs) * (1UL << SHOOTER_RESOLUTION_BITS)) /
      (1000000UL / SHOOTER_FREQUENCY_HZ));
}

void setShooterSpeed(uint8_t pin, int speed) {
  const uint16_t pulseUs = speed == 0
      ? SHOOTER_STOP_PULSE_US
      : map(speed, 1, 100, SHOOTER_MIN_PULSE_US, SHOOTER_MAX_PULSE_US);
  ledcWrite(pin, shooterPulseToDuty(pulseUs));
}

void setDeliverySpeed(int speed) {
  digitalWrite(DELIVERY_IN1_PIN, speed > 0 ? HIGH : LOW);
  digitalWrite(DELIVERY_IN2_PIN, LOW);
  const uint32_t maxDuty = (1UL << DELIVERY_RESOLUTION_BITS) - 1;
  ledcWrite(DELIVERY_PIN, map(speed, 0, 100, 0, maxDuty));
}

void handleStatus(AsyncWebServerRequest *request) {
  const auto statusJson = getStatusAsJson();
  request->send(200, "application/json", statusJson);
}

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Page not found");
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

void setup(void) {
  Serial.begin(115200);

  {
    pinMode(DELIVERY_IN1_PIN, OUTPUT);
    pinMode(DELIVERY_IN2_PIN, OUTPUT);
    digitalWrite(DELIVERY_IN1_PIN, LOW);
    digitalWrite(DELIVERY_IN2_PIN, LOW);

    const bool topOk = ledcAttachChannel(
        SHOOTER_TOP_PIN, SHOOTER_FREQUENCY_HZ, SHOOTER_RESOLUTION_BITS,
        SHOOTER_TOP_CHANNEL);
    const bool bottomOk = ledcAttachChannel(
        SHOOTER_BOTTOM_PIN, SHOOTER_FREQUENCY_HZ, SHOOTER_RESOLUTION_BITS,
        SHOOTER_BOTTOM_CHANNEL);
    const bool deliveryOk = ledcAttachChannel(
        DELIVERY_PIN, DELIVERY_FREQUENCY_HZ, DELIVERY_RESOLUTION_BITS,
        DELIVERY_CHANNEL);

    if (!topOk || !bottomOk || !deliveryOk) {
      Serial.println("Motor PWM initialization failed");
      while (true) delay(1000);
    }

    setShooterSpeed(SHOOTER_TOP_PIN, 0);
    setShooterSpeed(SHOOTER_BOTTOM_PIN, 0);
    setDeliverySpeed(0);
    delay(4000);
  }

  {
    if (!LittleFS.begin()) {
      Serial.println("LittleFS mount failed");
      return;
    }
    Serial.println("LittleFS mounted");
  }

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

    server.serveStatic("/", LittleFS, "/", "public, max-age=86401").setDefaultFile("index.html").setTryGzipFirst(true);

    server.begin();
  }
  Serial.println("HTTP server started");
}

void loop(void) {
  if (broadcastUpdates) {
    const auto updateStartTime = millis();
    broadcastUpdates = false;
    Serial.println("Reacting to broadcastUpdates");
    const auto status = getStatusAsJson();
    statusEvents.send(status, nullptr, millis());
    Serial.println(status);
    if (running) {
      setShooterSpeed(SHOOTER_TOP_PIN, shooter.top_speed);
      setShooterSpeed(SHOOTER_BOTTOM_PIN, shooter.bottom_speed);
      setDeliverySpeed(delivery.speed);
    } else {
      setShooterSpeed(SHOOTER_TOP_PIN, 0);
      setShooterSpeed(SHOOTER_BOTTOM_PIN, 0);
      setDeliverySpeed(0);
    }
    const auto updateElapsedTime = millis() - updateStartTime;
    Serial.print("Update applied in ");
    Serial.print(updateElapsedTime);
    Serial.println("ms");
  }
}
