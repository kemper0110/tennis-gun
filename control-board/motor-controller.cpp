#include <Arduino.h>
#include "motor-controller.h"

namespace {
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

uint32_t shooterPulseToDuty(uint16_t pulseUs) {
  return static_cast<uint32_t>(
      (static_cast<uint64_t>(pulseUs) * (1UL << SHOOTER_RESOLUTION_BITS)) /
      (1000000UL / SHOOTER_FREQUENCY_HZ));
}

void setShooterSpeed(uint8_t pin, uint8_t speed) {
  const uint16_t pulseUs = speed == 0
      ? SHOOTER_STOP_PULSE_US
      : map(speed, 1, 100, SHOOTER_MIN_PULSE_US, SHOOTER_MAX_PULSE_US);
  ledcWrite(pin, shooterPulseToDuty(pulseUs));
}

void setDeliveryMotorSpeed(uint8_t speed) {
  digitalWrite(DELIVERY_IN1_PIN, speed > 0 ? HIGH : LOW);
  digitalWrite(DELIVERY_IN2_PIN, LOW);
  const uint32_t maxDuty = (1UL << DELIVERY_RESOLUTION_BITS) - 1;
  ledcWrite(DELIVERY_PIN, map(speed, 0, 100, 0, maxDuty));
}
}  // namespace

void setupMotors() {
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

  stopMotors();
  delay(4000);
}

void applyMotorState(const DeviceState& state) {
  if (!state.running) {
    stopMotors();
    return;
  }
  setShooterSpeed(SHOOTER_TOP_PIN, state.topSpeed);
  setShooterSpeed(SHOOTER_BOTTOM_PIN, state.bottomSpeed);
  setDeliveryMotorSpeed(state.deliverySpeed);
}

void stopMotors() {
  setShooterSpeed(SHOOTER_TOP_PIN, 0);
  setShooterSpeed(SHOOTER_BOTTOM_PIN, 0);
  setDeliveryMotorSpeed(0);
}
