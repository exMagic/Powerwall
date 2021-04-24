#include <Arduino.h>

// #include <WiFi.h>
// #include <mDash.h>
// #include "secrets.h"
// #include "EspMQTTClient.h"

const int led = 2;
unsigned long previousMillis = 0;
const long interval = 1000;
bool ledState = 0; 

///MQTT
// EspMQTTClient client(
//   SECRET_SSID,
//   SECRET_PASS,
//   SECRET_MQTT_Broker_server,  // MQTT Broker server ip
//   SECRET_MQTT_USER,   // Can be omitted if not needed
//   SECRET_MQTT_PASSWORD,   // Can be omitted if not needed
//   SECRET_MQTT_CLIENT_NAME,     // Client name that uniquely identify your device
//   SECRET_MQTT_PORT              // The MQTT port, default to 1883. this line can be omitted
// );

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
int mVperAmp = 66;
///

// This function is called once everything is connected (Wifi and MQTT)
// WARNING : YOU MUST IMPLEMENT IT IF YOU USE EspMQTTClient
// void onConnectionEstablished()
// {
//   client.subscribe("vc/test", [](const String & payload) {
//     Serial.println(payload);
//   });
//   client.publish("vc/test", "//__START__//"); // You can activate the retain flag by setting the third parameter to true  
// }

/////////////////////////////__SETUP__//////////////////////////////
void setup() {
  Serial.begin(115200);
  // WiFi.begin(SECRET_SSID, SECRET_PASS);
  // mDashBegin(SECRET_DEVICE_PASSWORD);
  //  My setup onwards
  pinMode(led,  OUTPUT);

  // client.enableDebuggingMessages(); // Enable debugging messages sent to serial output
  // client.enableLastWillMessage("vc/test", "I am going offline");  // You can activate the retain flag by setting the third parameter to true
}

/////////////////////////////__LOOP__//////////////////////////////
void loop() {

  //client.loop();

  voltageSensorVal = analogRead(VPin);
  vOut = (voltageSensorVal / 4095) * vCC;
  V_Val =  vOut * factor + 0.6;

  ampsSensorVal = analogRead(Apin);
  Voltage = (ampsSensorVal / 4095.0) * 3300;
  A_Val = ((Voltage - ACSoffset) / mVperAmp);

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    ledState = not(ledState);
    digitalWrite(led,  ledState);
    //client.publish("vc", String(V_Val) + "," + String(A_Val));
    Serial.println(String(V_Val));
    Serial.println(String(A_Val));
  }
}