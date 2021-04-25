#include <Arduino.h>
#define MDASH_APP_NAME "Powerwall"
#include <mDash.h>
#include <WiFi.h>
#include "secrets.h"
#include "EspMQTTClient.h"

const int led = 2;
unsigned long previousMillis = 0;
const long interval = 1000;
bool ledState = 0;

///MQTT
EspMQTTClient client(
    SECRET_SSID,
    SECRET_PASS,
    SECRET_MQTT_Broker_server, // MQTT Broker server ip
    SECRET_MQTT_USER,          // Can be omitted if not needed
    SECRET_MQTT_PASSWORD,      // Can be omitted if not needed
    SECRET_MQTT_CLIENT_NAME,   // Client name that uniquely identify your device
    SECRET_MQTT_PORT           // The MQTT port, default to 1883. this line can be omitted
);

//// VOLTAGE
const int VPin = 35;
float voltageSensorVal;
float vOut;
float V_Val;
const float factor = 5.2;
const float vCC = 3.2;

//// AMPS
const int Apin = 34;
int ampsSensorVal = 0;
double Voltage = 0;
double A_Val = 0;
int ACSoffset = 2500;
double mVperAmp = 39.5;

//
const int numReadings2 = 100;
int readings2[numReadings2];      // the readings from the analog input
int readIndex2 = 0;              // the index of the current reading
int total2 = 0;                  // the running total
int average2 = 0;                // the average

const int numReadings = 100;
int readings[numReadings];      // the readings from the analog input
int readIndex = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average
///

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
void onConnectionEstablished()
{
  client.subscribe("vc/test", [](const String &payload) {
    Serial.println(payload);
  });
  client.publish("vc/test", "//__START__//"); // You can activate the retain flag by setting the third parameter to true
}

void setup()
{
  //Serial.begin(115200);
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  mDashBegin(SECRET_DEVICE_PASSWORD);
  pinMode(led, OUTPUT);
  client.enableDebuggingMessages();                              // Enable debugging messages sent to serial output
  client.enableLastWillMessage("vc/test", "I am going offline"); // You can activate the retain flag by setting the third parameter to true

  for (int thisReading = 0; thisReading < numReadings; thisReading++) {
    readings[thisReading] = 0;
  }
}

void loop()
{
  client.loop();

  // subtract the last reading:
  total2 = total2 - readings2[readIndex2];
  // read from the sensor:
  readings2[readIndex2] = analogRead(VPin);
  // add the reading to the total:
  total2 = total2 + readings2[readIndex2];
  // advance to the next position in the array:
  readIndex2 = readIndex2 + 1;
  // if we're at the end of the array...
  if (readIndex2 >= numReadings2) {
    // ...wrap around to the beginning:
    readIndex2 = 0;
  }
  // calculate the average:
  average2 = total2 / numReadings2;
  ////voltageSensorVal = analogRead(VPin);
  voltageSensorVal = average2;
  vOut = (voltageSensorVal / 4095) * vCC;
  V_Val = vOut * factor + 0.6;

  
  // subtract the last reading:
  total = total - readings[readIndex];
  // read from the sensor:
  readings[readIndex] = analogRead(Apin);
  // add the reading to the total:
  total = total + readings[readIndex];
  // advance to the next position in the array:
  readIndex = readIndex + 1;
  // if we're at the end of the array...
  if (readIndex >= numReadings) {
    // ...wrap around to the beginning:
    readIndex = 0;
  }
  // calculate the average:
  average = total / numReadings;
  //// ampsSensorVal = analogRead(Apin); //raw value
  ampsSensorVal = average; //average value
  Voltage = (ampsSensorVal / 4095.0) * 3300;
  A_Val = ((Voltage - ACSoffset) / mVperAmp);
  A_Val = (A_Val)*-1-24.52;




  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval)
  {
    previousMillis = currentMillis;
    ledState = not(ledState);
    digitalWrite(led, ledState);
    client.publish("vc", String(V_Val) + "," + String(A_Val) + " rv:" + String(analogRead(Apin)) + " av:" + String(average));
  }
}