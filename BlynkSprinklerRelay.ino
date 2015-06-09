/**************************************************************
 * Blynk is a platform with iOS and Android apps to control
 * Arduino, Raspberry Pi and the likes over the Internet.
 * You can easily build graphic interfaces for all your
 * projects by simply dragging and dropping widgets.
 *
 *   Downloads, docs, tutorials: http://www.blynk.cc
 *   Blynk community:            http://community.blynk.cc
 *   Social groups:              http://www.fb.com/blynkapp
 *                               http://twitter.com/blynk_app
 *
 * Blynk library is licensed under MIT license
 * This example code is in public domain.
 *
 **************************************************************
 * This example runs directly on ESP8266 chip.
 *
 * You need to install this for ESP8266 development:
 *   https://github.com/esp8266/Arduino
 *
 * Change WiFi ssid, pass, and Blynk auth token to run :)
 *
 **************************************************************/

#define BLYNK_PRINT Serial    // Comment this out to disable prints and save space
#define BLYNK_EXPERIMENTAL

#define TIME_SECONDS 1000
#define TIME_MINUTES 60000

#define TIME_UNIT TIME_MINUTES

#include <ESP8266WiFi.h>
#include <BlynkSimpleEsp8266.h>

#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WifiManager.h>

#include <SimpleTimer.h>
#include <DHT.h>

//either create a config.h and define auth token there
//or comment and include here like: 
//char auth[] = "BLYNK_AUTH_TOKEN";
#include "Config.h"

WiFiManager wifi(0);

SimpleTimer timer;
int timerId;

#define DHTPIN 4 // what pin we're connected to
#define DHTTYPE DHT22 // DHT 11
DHT dht(DHTPIN, DHTTYPE, 20);


//WidgetLED led1(1);

boolean started = false;
long wateringStart = 0;
long wateringLength = 0;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).

void setup()
{

  Serial.begin(115200);
  wifi.autoConnect("Blynk");
  String ssid = wifi.getSSID();
  String pass = wifi.getPassword();

  Blynk.begin(auth, ssid.c_str(), pass.c_str());
  //Blynk.begin(auth, ssid.c_str(), "");

  pinMode(5, OUTPUT);
  digitalWrite(5, LOW);

  timer.setInterval(1000, heartBeat);

  dht.begin();
  
  Serial.println("done setup");
}


void loop()
{
  Blynk.run();
  timer.run();
  if (!started && Blynk.connected()) {
    //intial setup after connected
    started = true;
    for (int i = 0; i < 7; i++) {
      Blynk.virtualWrite(1, i % 2);
      delay(200);
    }
  }
}

void heartBeat() {
  if ( wateringLength != 0 ) {
    if ((wateringStart + wateringLength - millis()) < 1000) {
      //wateringLength = 0;
      Serial.println("done watering");
      digitalWrite(5, LOW);
      wateringStart = 0;
      int i = digitalRead(5);
      Blynk.virtualWrite(1, i);
      Blynk.push_notification("watering done");
    }
  }
}

// This takes the value of virtual pin 0 in blynk program...  if it is 1 turns on, else off...
BLYNK_WRITE(0) {
  int a = param.asInt();
  if (a == 0) {
    digitalWrite(5, LOW);

    wateringStart = 0;
  } else {
    digitalWrite(5, HIGH);
    if (wateringStart == 0) {
      wateringStart = millis();
      //Blynk.virtualRead(6);
    }
  }
  Blynk.virtualWrite(1, a);
  Serial.println("write");
}

//// This publishes the int power back to the blynk program...
//UPTIME V2
BLYNK_READ(2) {
  //Serial.println("read");

  long uptime = millis() / TIME_UNIT;
  Blynk.virtualWrite(2, uptime);
}

//REFRESH BUTTON v3
BLYNK_WRITE(3) {
  int a = param.asInt();
  if (a != 0) {
    Serial.print("refresh");
    int i = digitalRead(5);
    Blynk.virtualWrite(1, i);
  }
}

//WATERING RUN TIME GAUGE V4
BLYNK_READ(4) {
  //Serial.println("read watering interval");

  long watering = 0;
  if ( wateringStart != 0 ) {
    watering = (millis() - wateringStart) / TIME_UNIT ;
  }

  Blynk.virtualWrite(4, watering);
}

//WATERING TIME LEFT V5
BLYNK_READ(5) {
  //Serial.println("read");

  long remaining = 0;
  if ( wateringLength != 0 && wateringStart != 0) {
    remaining = (wateringStart + wateringLength - millis()) / TIME_UNIT ;
  } else {
    remaining = wateringLength / TIME_UNIT;
  }

  Blynk.virtualWrite(5, remaining);
}

//WATERING TIME INTERVAL SLIDER V6
BLYNK_WRITE(6) {
  //Serial.println("write watering length");
  long a = param.asLong();
  //Serial.print("write legth");
  wateringLength = a * TIME_UNIT;
}

//SCHEDULED TIMER ON/OFF V10
BLYNK_WRITE(10) {
  int a = param.asInt();
  if (a == 0) {
    digitalWrite(5, LOW);

    wateringStart = 0;
  } else {
    digitalWrite(5, HIGH);
    if (wateringStart == 0) {
      wateringStart = millis();
    }
  }
  Blynk.virtualWrite(1, a);
  Serial.println("write");
}

//RESET ESP V20
BLYNK_WRITE(20) {
  int a = param.asInt();
  if (a == 0) {
    Serial.println("reset");
    ESP.reset();
  }
}

BLYNK_READ(30) {
  //Serial.println("read");
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  char charVal[4];
  dtostrf(t, 5, 1, charVal);
  Blynk.virtualWrite(30, charVal);
  Blynk.virtualWrite(29, charVal);
  dtostrf(h, 5, 2, charVal);
  Blynk.virtualWrite(31, charVal);
}


