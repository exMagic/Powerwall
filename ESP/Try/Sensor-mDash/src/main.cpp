#include <Arduino.h>
#define MDASH_APP_NAME "Powerwall"
#include <mDash.h>
#include <WiFi.h>
#include "secrets.h"

const int led = 2;
unsigned long previousMillis = 0;
const long interval = 100;
bool ledState = 0;

void setup()
{
  Serial.begin(115200);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  mDashBegin(SECRET_DEVICE_PASSWORD);
  pinMode(led, OUTPUT);
}

void loop()
{
  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    ledState = not(ledState);
    digitalWrite(led, ledState);
  }
}