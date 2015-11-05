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
//#define BLYNK_DEBUG
#define BLYNK_PRINT Serial        // Comment this out to disable prints and save space
#define BLYNK_EXPERIMENTAL

#define TIME_SECONDS 1000
#define TIME_MINUTES 60000

#define TIME_UNIT TIME_MINUTES
//#define TIME_UNIT TIME_SECONDS

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <BlynkSimpleEsp8266.h>   //https://github.com/blynkkk/blynk-library

#include <EEPROM.h>
#include <ESP8266mDNS.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <SimpleTimer.h>          //http://playground.arduino.cc/Code/SimpleTimer  || https://github.com/jfturcot/SimpleTimer
#include <DHT.h>

//either create a config.h and define auth token there
//or comment and include here like:
//char auth[] = "BLYNK_AUTH_TOKEN";
#include "Config.h"


SimpleTimer timer;
int timerId;
unsigned long counter = 0;

float h = 0;
float t = 0;

//DHT22 config
#define DHTPIN 4 // what pin DHT is connected to
#define DHTTYPE DHT22 // DHT 11
DHT dht(DHTPIN, DHTTYPE, 20);

boolean started = false;

int selected = 0;

const int channelPins[4]          = {5, 4, 2, 0};
const int channelRemainingPins[4] = {1, 2, 3, 4};
const int channelStatusPins[4]    = {5, 6, 7, 8};
unsigned long wateringStart[4]    = {0, 0, 0, 0};
unsigned long wateringLength[4]   = {0, 0, 0, 0};
String statuses[4]                = {"", "", "", ""};
//String status = "boot";


const int CMD_WAIT = 0;
const int CMD_START = 1;
const int CMD_STOP = 2;
const int CMD_STOP_SELECTED = 3;
const int CMD_STOP_EXPIRED = 4;

int state = CMD_WAIT;

// You should get Auth Token in the Blynk App.
// Go to the Project Settings (nut icon).


void sendData() {
  Serial.print("connecting to ");
  Serial.println(host);

  WiFiClient emoClient;

  const int httpPort = 80;
  if (!emoClient.connect(host, httpPort)) {
    Serial.println("connection failed");
    return;
  }

  String url = "/input/post.json?node=";
  url += nodeId;
  url += "&apikey=";
  url += privateKey;
  url += "&json={temperature:";
  url += t;
  url += ",humidity:";
  url += h;
  url += "}";

  Serial.print("Requesting URL: ");
  Serial.println(url);

  // This will send the request to the server
  emoClient.print(String("GET ") + url + " HTTP/1.1\r\n" +
                  "Host: " + host + "\r\n" +
                  "Connection: close\r\n\r\n");
  delay(10);

  // Read all the lines of the reply from server and print them to Serial
  while (emoClient.available()) {
    String line = emoClient.readStringUntil('\r');
    Serial.print(line);
  }

  Serial.println();
  Serial.println("closing connection");
}


void heartBeat() {
  counter++;

  int i = counter % 4;
  unsigned long remaining = 0;
  if (wateringStart[i] > 0) {
    remaining = (wateringStart[i] + wateringLength[i] - millis()) / TIME_UNIT ;
  }
  //Blynk.virtualWrite(19, remaining);
  Blynk.virtualWrite(channelRemainingPins[i], remaining);


  if (wateringStart[i] > 0 && wateringStart[i] + wateringLength[i] < millis()) {
    //done watering
    state = CMD_STOP_EXPIRED;
  }


  if (counter % 60 == 0) {
    h = dht.readHumidity();
    t = dht.readTemperature();

    if (!isnan(h) && !isnan(t)) {
      sendData();
    }
  }
}

void setStatus(int channel, String s) {
  statuses[channel] = s;
  Blynk.virtualWrite(channelStatusPins[channel], s);
}


void startWatering(int channel, int wl) {
  wateringLength[channel] = wl * TIME_UNIT;
  state = CMD_START;
}

void stopWatering(int channel) {
  digitalWrite(channelPins[channel], LOW);
  wateringStart[channel] = 0;
  wateringLength[channel] = 0;
  setStatus(channel, "  off");
  Blynk.virtualWrite(channelRemainingPins[channel], 0);

}

void setup()
{

  Serial.begin(115200);
  
  WiFiManager wifi;

  //taking care of wifi connection
  wifi.autoConnect("Blynk");
  //String ssid = wifi.getSSID();
  //String pass = wifi.getPassword();

  //config blynk
  Blynk.config(auth);

  //setup pins
  for (int i = 0; i < 4; i++) {
    pinMode(channelPins[i], OUTPUT);
    digitalWrite(channelPins[i], LOW);
    delay(10);
  }

  //setup timers
  timer.setInterval(1000, heartBeat);

  //setup hardware
  dht.begin();

  Serial.println("done setup");
}


void loop()
{
  //blynk connect and run loop
  Blynk.run();
  //timer run loop
  timer.run();

  //run once after blynk is connected
  if (!started && Blynk.connected()) {
    started = true;
    //setStatus(0, status);
    setStatus(0, "Node");
    yield();
    setStatus(1, "has");
    yield();
    setStatus(2, "boot");
    yield();
    setStatus(3, "ed");
    yield();
    Blynk.virtualRead(0);
    yield();
  }

  //  if (Blynk.connected()) {

  if (state != CMD_WAIT) {
    Serial.println("new command");
    switch (state) {
      case CMD_START:
        Serial.println("start water");
        digitalWrite(channelPins[selected], HIGH);
        if (wateringStart[selected] == 0) {
          wateringStart[selected] = millis();
          setStatus(selected, "   ON");
        }
        break;
      case CMD_STOP:
        Serial.println("stop all");
        for (int i = 0; i < 4; i++) {
          //done watering
          stopWatering(i);
        }
        break;
      case CMD_STOP_SELECTED:
        Serial.println("stop selected");
        //done watering
        stopWatering(selected);
        break;
      case CMD_STOP_EXPIRED:
        Serial.println("stop expired");
        for (int i = 0; i < 4; i++) {
          if (wateringStart[i] > 0 && wateringStart[i] + wateringLength[i] < millis()) {
            //done watering
            stopWatering(i);
          }
        }
        break;
    }
    //reset state
    state = CMD_WAIT;
  }
  //  }
  //TODO:
  //timer with auto timeout
  //check to see when we lost connectivity and reboot after a set time


}




BLYNK_WRITE(0) {
  Serial.println("selected");
  int a = param.asInt();
  selected = a - 1;
}

BLYNK_WRITE(11) {
  int a = param.asInt();
  if (a != 0) {
    startWatering(selected, 30);
  }
}

BLYNK_WRITE(12) {
  int a = param.asInt();
  if (a != 0) {
    startWatering(selected, 60);
  }
}

BLYNK_WRITE(13) {
  int a = param.asInt();
  if (a != 0) {
    startWatering(selected, 90);
  }
}

BLYNK_WRITE(14) {
  int a = param.asInt();
  if (a != 0) {
    startWatering(selected, 120);
  }
}

//status indicator - value display widget
BLYNK_READ(5) {
  Blynk.virtualWrite(5, statuses[0]);
}
BLYNK_READ(6) {
  Blynk.virtualWrite(6, statuses[1]);
}
BLYNK_READ(7) {
  Blynk.virtualWrite(7, statuses[2]);
}
BLYNK_READ(8) {
  Blynk.virtualWrite(8, statuses[3]);
}

//stop all - button
BLYNK_WRITE(9) {
  int a = param.asInt();
  if (a != 0) {
    state = CMD_STOP;
  }
}

//stop - button
BLYNK_WRITE(18) {
  int a = param.asInt();
  if (a != 0) {
    state = CMD_STOP_SELECTED;
  }
}

//time left indicator - value display widget
/*BLYNK_READ(19) {
  unsigned long remaining = 0;
  if (wateringStart[0] > 0) {
    remaining = (wateringStart[0] + wateringLength[0] - millis()) / TIME_UNIT ;
  }
  Blynk.virtualWrite(19, remaining);
}*/

BLYNK_READ(30) {
  long uptime = millis() / TIME_UNIT;
  Blynk.virtualWrite(30, uptime);
}


//RESET ESP V31 - Button
BLYNK_WRITE(31) {
  int a = param.asInt();
  if (a != 0) {
    Serial.println("reset");
    ESP.reset();
    delay(1000);
  }
}



