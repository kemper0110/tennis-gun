#include <Arduino.h>

// This test expects Arduino IDE -> Tools -> USB CDC On Boot -> Enabled.
// On ESP32-C3, Serial then uses the serial side of the built-in composite
// USB Serial/JTAG device, while its other USB interface remains available
// to the debugger.

// AirM2M CORE ESP32-C3 connections used by the main firmware.
constexpr uint8_t SHOOTER_TOP_PIN = 4;
constexpr uint8_t SHOOTER_BOTTOM_PIN = 5;

// HW-166 / TB6612FNG, channel B.
// Connect STBY to 3.3 V (HIGH), VCC to 3.3 V and all grounds together.
constexpr uint8_t DELIVERY_PWM_PIN = 6;   // PWMB
constexpr uint8_t DELIVERY_IN1_PIN = 7;   // BIN1
constexpr uint8_t DELIVERY_IN2_PIN = 3;   // BIN2

// ESP32-C3 LEDC channels 0 and 1 share timer 0, which is intentional:
// both SimonK ESCs use the same frequency and resolution. Channel 2 uses
// timer 1, so the DC motor can have a different PWM frequency.
constexpr uint8_t SHOOTER_TOP_CHANNEL = 0;
constexpr uint8_t SHOOTER_BOTTOM_CHANNEL = 1;
constexpr uint8_t DELIVERY_CHANNEL = 2;

constexpr uint32_t ESC_FREQUENCY_HZ = 50;
constexpr uint8_t ESC_RESOLUTION_BITS = 14;
constexpr uint32_t ESC_PERIOD_US = 1000000UL / ESC_FREQUENCY_HZ;
constexpr uint16_t ESC_MIN_PULSE_US = 1000;
constexpr uint16_t ESC_MAX_PULSE_US = 2000;
constexpr uint32_t ESC_ARMING_TIME_MS = 4000;
constexpr uint32_t MOTOR_COMMAND_TIMEOUT_MS = 10000;

constexpr uint32_t DELIVERY_FREQUENCY_HZ = 20000;
constexpr uint8_t DELIVERY_RESOLUTION_BITS = 10;
constexpr uint32_t DELIVERY_MAX_DUTY =
    (1UL << DELIVERY_RESOLUTION_BITS) - 1;

int topPercent = 0;
int bottomPercent = 0;
int deliveryPercent = 0;
uint32_t lastMotorCommandMs = 0;
bool safetyTimeoutActive = false;

uint32_t pulseUsToDuty(uint16_t pulseUs) {
  return static_cast<uint32_t>(
      (static_cast<uint64_t>(pulseUs) * (1UL << ESC_RESOLUTION_BITS)) /
      ESC_PERIOD_US);
}

uint16_t percentToPulseUs(int percent) {
  return static_cast<uint16_t>(map(percent, 0, 100,
                                   ESC_MIN_PULSE_US, ESC_MAX_PULSE_US));
}

bool writeEsc(uint8_t pin, int percent) {
  const uint16_t pulseUs = percentToPulseUs(percent);
  return ledcWrite(pin, pulseUsToDuty(pulseUs));
}

void setTop(int percent) {
  topPercent = constrain(percent, 0, 100);
  const bool ok = writeEsc(SHOOTER_TOP_PIN, topPercent);
  Serial.printf("top = %d%%, pulse = %u us, write = %s\n",
                topPercent, percentToPulseUs(topPercent), ok ? "OK" : "ERROR");
}

void setBottom(int percent) {
  bottomPercent = constrain(percent, 0, 100);
  const bool ok = writeEsc(SHOOTER_BOTTOM_PIN, bottomPercent);
  Serial.printf("bottom = %d%%, pulse = %u us, write = %s\n",
                bottomPercent, percentToPulseUs(bottomPercent),
                ok ? "OK" : "ERROR");
}

void setDelivery(int percent) {
  percent = constrain(percent, -100, 100);

  // Remove PWM before changing the H-bridge direction.
  ledcWrite(DELIVERY_PWM_PIN, 0);
  delay(5);

  if (percent > 0) {
    digitalWrite(DELIVERY_IN1_PIN, HIGH);
    digitalWrite(DELIVERY_IN2_PIN, LOW);
  } else if (percent < 0) {
    digitalWrite(DELIVERY_IN1_PIN, LOW);
    digitalWrite(DELIVERY_IN2_PIN, HIGH);
  } else {
    // Coast/standby for the selected bridge channel.
    digitalWrite(DELIVERY_IN1_PIN, LOW);
    digitalWrite(DELIVERY_IN2_PIN, LOW);
  }

  deliveryPercent = percent;
  const uint32_t duty =
      (DELIVERY_MAX_DUTY * static_cast<uint32_t>(abs(percent))) / 100;
  const bool ok = ledcWrite(DELIVERY_PWM_PIN, duty);
  Serial.printf("dc = %d%%, duty = %lu/%lu, write = %s\n",
                deliveryPercent, static_cast<unsigned long>(duty),
                static_cast<unsigned long>(DELIVERY_MAX_DUTY),
                ok ? "OK" : "ERROR");
}

void stopAll() {
  // Minimum valid throttle keeps the ESCs armed but stops the BLDC motors.
  setTop(0);
  setBottom(0);
  setDelivery(0);
  safetyTimeoutActive = false;
  Serial.println("All motors stopped");
}

void startSafetyTimeout() {
  lastMotorCommandMs = millis();
  safetyTimeoutActive =
      topPercent != 0 || bottomPercent != 0 || deliveryPercent != 0;
}

void printStatus() {
  Serial.println("--- status ---");
  Serial.printf("top:    %d%%, freq=%lu Hz, duty=%lu\n",
                topPercent,
                static_cast<unsigned long>(ledcReadFreq(SHOOTER_TOP_PIN)),
                static_cast<unsigned long>(ledcRead(SHOOTER_TOP_PIN)));
  Serial.printf("bottom: %d%%, freq=%lu Hz, duty=%lu\n",
                bottomPercent,
                static_cast<unsigned long>(ledcReadFreq(SHOOTER_BOTTOM_PIN)),
                static_cast<unsigned long>(ledcRead(SHOOTER_BOTTOM_PIN)));
  // Arduino-ESP32 3.1.3 reports readFreq=0 when duty is zero.
  Serial.printf("dc:     %d%%, configuredFreq=%lu Hz, readFreq=%lu Hz, duty=%lu, IN1=%d, IN2=%d\n",
                deliveryPercent,
                static_cast<unsigned long>(DELIVERY_FREQUENCY_HZ),
                static_cast<unsigned long>(ledcReadFreq(DELIVERY_PWM_PIN)),
                static_cast<unsigned long>(ledcRead(DELIVERY_PWM_PIN)),
                digitalRead(DELIVERY_IN1_PIN),
                digitalRead(DELIVERY_IN2_PIN));
}

void printHelp() {
  Serial.println();
  Serial.println("Commands (set Serial Monitor line ending to Newline):");
  Serial.println("  top 0..100       - top BLDC speed");
  Serial.println("  bottom 0..100    - bottom BLDC speed");
  Serial.println("  both 0..100      - both BLDC speeds");
  Serial.println("  dc -100..100     - DC speed and direction");
  Serial.println("  arm              - stop and arm both ESCs for 4 seconds");
  Serial.println("  stop             - immediately stop all motors");
  Serial.println("  status           - show LEDC frequency and duty");
  Serial.println("  help             - show this help");
  Serial.println();
  Serial.println("Start with 5-10%. Keep 'stop' ready.");
  Serial.println("Any non-zero command is stopped automatically after 10 seconds.");
}

void armEscs() {
  Serial.println("Arming ESCs at minimum throttle...");
  stopAll();
  delay(ESC_ARMING_TIME_MS);
  Serial.println("ESC arming interval completed");
}

void processCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command.isEmpty()) {
    return;
  }

  if (command == "stop") {
    stopAll();
    return;
  }
  if (command == "status") {
    printStatus();
    return;
  }
  if (command == "help") {
    printHelp();
    return;
  }
  if (command == "arm") {
    armEscs();
    return;
  }

  char name[16] = {};
  int value = 0;
  if (sscanf(command.c_str(), "%15s %d", name, &value) != 2) {
    Serial.println("Invalid command. Type 'help'.");
    return;
  }

  if (strcmp(name, "top") == 0 && value >= 0 && value <= 100) {
    setTop(value);
    startSafetyTimeout();
  } else if (strcmp(name, "bottom") == 0 && value >= 0 && value <= 100) {
    setBottom(value);
    startSafetyTimeout();
  } else if (strcmp(name, "both") == 0 && value >= 0 && value <= 100) {
    setTop(value);
    setBottom(value);
    startSafetyTimeout();
  } else if (strcmp(name, "dc") == 0 && value >= -100 && value <= 100) {
    setDelivery(value);
    startSafetyTimeout();
  } else {
    Serial.println("Unknown command or value outside the allowed range.");
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(50);
  delay(1000);

  pinMode(DELIVERY_IN1_PIN, OUTPUT);
  pinMode(DELIVERY_IN2_PIN, OUTPUT);
  digitalWrite(DELIVERY_IN1_PIN, LOW);
  digitalWrite(DELIVERY_IN2_PIN, LOW);

  const bool topOk = ledcAttachChannel(
      SHOOTER_TOP_PIN, ESC_FREQUENCY_HZ, ESC_RESOLUTION_BITS,
      SHOOTER_TOP_CHANNEL);
  const bool bottomOk = ledcAttachChannel(
      SHOOTER_BOTTOM_PIN, ESC_FREQUENCY_HZ, ESC_RESOLUTION_BITS,
      SHOOTER_BOTTOM_CHANNEL);
  const bool deliveryOk = ledcAttachChannel(
      DELIVERY_PWM_PIN, DELIVERY_FREQUENCY_HZ, DELIVERY_RESOLUTION_BITS,
      DELIVERY_CHANNEL);

  Serial.printf("LEDC attach: top=%s, bottom=%s, dc=%s\n",
                topOk ? "OK" : "ERROR",
                bottomOk ? "OK" : "ERROR",
                deliveryOk ? "OK" : "ERROR");

  if (!topOk || !bottomOk || !deliveryOk) {
    digitalWrite(DELIVERY_IN1_PIN, LOW);
    digitalWrite(DELIVERY_IN2_PIN, LOW);
    Serial.println("Initialization failed. Motors will not be started.");
    while (true) {
      delay(1000);
    }
  }

  armEscs();
  printStatus();
  printHelp();
}

void loop() {
  if (Serial.available() > 0) {
    processCommand(Serial.readStringUntil('\n'));
  }

  if (safetyTimeoutActive &&
      millis() - lastMotorCommandMs >= MOTOR_COMMAND_TIMEOUT_MS) {
    Serial.println("Safety timeout: stopping all motors");
    stopAll();
  }
}
