#include <Arduino.h>
#include "driver/ledc.h"
#include "esp_err.h"
#include <ESP32Servo.h>

Servo topServo, bottomServo;


// const auto TOP_PIN = 4;
// const auto BOTTOM_PIN = 5;
const auto DELIVERY_PIN = 6;
const auto IN1_PIN = 3;
const auto IN2_PIN = 7;


void initPwm(ledc_timer_t timer, ledc_channel_t channel, int pin) {
  ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .duty_resolution = LEDC_TIMER_13_BIT,
    .timer_num = timer,
    .freq_hz = 4000,
    .clk_cfg = LEDC_AUTO_CLK
  };
  Serial.print("ledc timer config ");
  Serial.println(ledc_timer_config(&ledc_timer));

  ledc_channel_config_t ledc_channel = {
    .gpio_num = pin,
    .speed_mode = LEDC_LOW_SPEED_MODE,
    .channel = channel,
    .intr_type = LEDC_INTR_DISABLE,
    .timer_sel = timer,
    .duty = 0,
    .hpoint = 0
  };
  Serial.print("ledc channel config ");
  Serial.println(ledc_channel_config(&ledc_channel));

  Serial.print("update duty ");
  Serial.println(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));

  Serial.print("get duty ");
  Serial.println(ledc_get_duty(LEDC_LOW_SPEED_MODE, channel));

  Serial.print("set duty ");
  Serial.println(ledc_set_duty(LEDC_LOW_SPEED_MODE, channel, 4096));

  Serial.print("update duty ");
  Serial.println(ledc_update_duty(LEDC_LOW_SPEED_MODE, channel));

  Serial.print("get duty ");
  Serial.println(ledc_get_duty(LEDC_LOW_SPEED_MODE, channel));
}


void setup(void) {
  Serial.begin(115200);
  Serial.println("board start");

  // topServo.attach(TOP_PIN);
  // bottomServo.attach(BOTTOM_PIN);
  // Serial.println(ledcAttach(DELIVERY_PIN, 50, 8));

  pinMode(IN1_PIN, OUTPUT);
  pinMode(IN2_PIN, OUTPUT);
  pinMode(DELIVERY_PIN, OUTPUT);

  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, LOW);

  // Serial.println(ledcAttach(TOP_PIN, 50, 8));
  // Serial.println(ledcAttach(BOTTOM_PIN, 50, 8));
  // Serial.println(ledcAttach(DELIVERY_PIN, 50, 8));

  // const auto channel = 0;
  // for (const auto pin : { 3, 4, 5, 6, 7, 8, 11, 12, 13 }) {
  //   Serial.println(String("pin ") + pin);
  //   Serial.println(ledcAttachChannel(pin, 5000, 8, channel));
  //   Serial.println(ledcWrite(pin, 80));
  //   Serial.println(ledcRead(pin));
  // }

  // Serial.println(ledcRead(3));
  // Serial.println(ledcWrite(3, 80));
  // Serial.println(ledcRead(3));

  // initPwm(LEDC_TIMER_0, LEDC_CHANNEL_0, 4);
  // initPwm(LEDC_TIMER_0, LEDC_CHANNEL_0, 5);
  // initPwm(LEDC_TIMER_0, LEDC_CHANNEL_1, 5);
  // initPwm(LEDC_TIMER_1, LEDC_CHANNEL_2, 6);
  // initPwm(LEDC_TIMER_2, LEDC_CHANNEL_3, 7);


  // ledc_timer_config_t ledc_timer1 = {
  //   .speed_mode = LEDC_LOW_SPEED_MODE,
  //   .duty_resolution = LEDC_TIMER_8_BIT,
  //   .timer_num = LEDC_TIMER_1,
  //   .freq_hz = 4000,
  //   .clk_cfg = LEDC_AUTO_CLK
  // };
  // Serial.print("ledc timer config ");
  // Serial.println(ledc_timer_config(&ledc_timer1));

  // ledc_timer_config_t ledc_timer2 = {
  //   .speed_mode = LEDC_LOW_SPEED_MODE,
  //   .duty_resolution = LEDC_TIMER_8_BIT,
  //   .timer_num = LEDC_TIMER_2,
  //   .freq_hz = 4000,
  //   .clk_cfg = LEDC_AUTO_CLK
  // };
  // Serial.print("ledc timer config ");
  // Serial.println(ledc_timer_config(&ledc_timer2));


  // ledc_channel_config_t ledc_channel1 = {
  //   .gpio_num = SHOOTER_TOP_PIN,
  //   .speed_mode = LEDC_LOW_SPEED_MODE,
  //   .channel = SHOOTER_TOP_CHANNEL,
  //   .intr_type = LEDC_INTR_DISABLE,
  //   .timer_sel = LEDC_TIMER_1,
  //   .duty = 50,
  //   .hpoint = 0
  // };
  // Serial.print("ledc channel config ");
  // Serial.println(ledc_channel_config(&ledc_channel1));



  // ledc_channel_config_t ledc_channel2 = {
  //   .gpio_num = SHOOTER_BOTTOM_PIN,
  //   .speed_mode = LEDC_LOW_SPEED_MODE,
  //   .channel = SHOOTER_BOTTOM_CHANNEL,
  //   .intr_type = LEDC_INTR_DISABLE,
  //   .timer_sel = LEDC_TIMER_1,
  //   .duty = 50,
  //   .hpoint = 0
  // };
  // Serial.print("ledc channel config ");
  // Serial.println(ledc_channel_config(&ledc_channel2));

  // Serial.print("get duty ");
  // Serial.println(ledc_get_duty(LEDC_LOW_SPEED_MODE, SHOOTER_TOP_CHANNEL));
  // Serial.print("get duty2 ");
  // Serial.println(ledc_get_duty(LEDC_LOW_SPEED_MODE, SHOOTER_BOTTOM_CHANNEL));

  Serial.println("board started");
}

void loop(void) {
  Serial.print("up start; ");
  
  digitalWrite(IN1_PIN, LOW);
  digitalWrite(IN2_PIN, HIGH);
  for (int i = 0; i < 180; ++i) {
    Serial.print(i);
    Serial.print(" ");
    // topServo.write(i);
    // bottomServo.write(i);
    analogWrite(DELIVERY_PIN, i);
    // ledcWrite(DELIVERY_PIN, i);
    // deliveryServo.write(i);
    // ledcWrite(TOP_PIN, i);
    // ledcWrite(BOTTOM_PIN, i);
    // ledcWrite(DELIVERY_PIN, i);
    delay(10);
  }
  digitalWrite(IN1_PIN, HIGH);
  digitalWrite(IN2_PIN, LOW);
  Serial.println();
  for (int i = 180; i > 0; --i) {
    Serial.print(i);
    Serial.print(" ");
    analogWrite(DELIVERY_PIN, i);
    // ledcWrite(DELIVERY_PIN, i);
    // topServo.write(i);
    // bottomServo.write(i);
    // deliveryServo.write(i);
    // ledcWrite(TOP_PIN, i);
    // ledcWrite(BOTTOM_PIN, i);
    // ledcWrite(DELIVERY_PIN, i);
    delay(10);
  }
  Serial.println();
  delay(400);
}