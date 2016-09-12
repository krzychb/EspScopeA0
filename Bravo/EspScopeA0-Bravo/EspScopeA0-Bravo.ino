/*
  This application is sampling analog input A0 of ESP8266
  so it then can be displayed on-line in a web browser
  Application consists of several files
  Please check repository below for details

  Repository: https://github.com/krzychb/EspScopeA0
  Version: Bravo
  File: EspScopeA0-Bravo.ino
  Revision: 0.2.4
  Date: 21-Sep-2016
  Author: krzychb at gazeta.pl

  Copyright (c) 2016 Krzysztof Budzynski. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include <ESP8266WiFi.h>

const char* ssid = "********";
const char* password = "********";

//
// Logging to ThingSpeak
//
#include <WiFiClient.h>
#define UPDATE_PERIOD 60000
#define TIMEOUT_PERIOD 1000
#define DATA_HOST "api.thingspeak.com"
#define API_KEY "your-api-key-here"

#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif


// tracking of number of Wi-Fi reconnects
// and connection time
unsigned long numberOfReconnects;
unsigned long millisReconnected;
unsigned long timeConnected;
//
// Continuous sampling rate of A0 in this application is about 10 samples/ms
// Wi-Fi connection gets stuck if continuous A0 sampling is not paused
// Minimum pause duration depends on number of samples, for instance:
// 100 samples, pause 5ms
// 600 samples, pause 13ms
// Above values have been established experimentally using this application
// Check them out on your own - they may depend on version of SDK
//
// Ref: https://github.com/esp8266/Arduino/issues/1634
//

#define NUMBER_OF_SAMPLES 2000  // maximum number of samples taken
int samples[NUMBER_OF_SAMPLES];
int numberOfSamples = 20;
unsigned long samplingTime = 1;  // time in us to collect all the numberOfSamples
int samplingPause = 1; // time in ms to pause between continuous sampling

unsigned long totalSamples;
unsigned long millisLastSample;


void analogSample(void)
{
  // do not start sampling before pause time expires
  if (millis() < millisLastSample + samplingPause)
  {
    return;
  }
  samplingTime = micros();
  for (int i = 0; i < numberOfSamples; i++)
  {
    samples[i] = analogRead(A0);
    totalSamples++;
  }
  samplingTime = micros() - samplingTime;
  millisLastSample = millis();
}


unsigned long showStatisticsTimer;
void showStatistics(void)
{
  // show statistics every 60s (60000ms)
  if(millis() > showStatisticsTimer + 60000)
  {
    showStatisticsTimer = millis();
    Serial.print(numberOfSamples);
    Serial.print(" : ");
    // average number of samples per second excluding pause time
    Serial.print(numberOfSamples * 1000000 / samplingTime);
    Serial.print(" 1/s : ");
    Serial.print(samplingPause);
    Serial.print("ms : ");
    Serial.print(numberOfReconnects);
    Serial.print(" : ");
    timeConnected = (millis() - millisReconnected) / 1000;
    Serial.print(timeConnected);
    Serial.print("s : ");
    // average number of samples per second including pause time
    // since last connected / reconnected to Wi-Fi
    Serial.print(totalSamples / timeConnected);
    Serial.print(" 1/s");
    Serial.println();
  }
}


void logToThingSpeak(void)
{
  WiFiClient client;
  static long dataLogTimer;
  static long transmissionAttempts;
  static long replyTimeouts;
  static long connectionFailures;
  static long postingTime = 0;

  Serial.printf("DATALOG > %s ", DATA_HOST);
  // Wait if this is too soon to send data
  while (millis() -  dataLogTimer < UPDATE_PERIOD)
  {
    Serial.print(".");
    delay(500);
  }
  Serial.print(" > ");

  dataLogTimer = millis();
  transmissionAttempts++;

  // connect to the host
  if (!client.connect(DATA_HOST, 80))
  {
    Serial.print("Connection failure!");
    connectionFailures++;
    return;
  }

  Serial.print("Sending > ");

  // send the data to the host
  client.print(
    String("GET /update?key=") + API_KEY +
          "&field1=" + String (dataLogTimer / 60000) +
          "&field2=" + String (postingTime) +
          "&field3=" + String (connectionFailures) +
          "&field4=" + String (numberOfReconnects) +
          "&field5=" + String (timeConnected) +
          "&field6=" + String (totalSamples) +
          "&field7=" + String (samplingPause) +
          "&field8=" + String (numberOfSamples) +
          " " +
          "HTTP/1.1\r\n" +
          "Host: " + DATA_HOST + "\r\n" +
          "Connection: close\r\n\r\n");

  // wait for reply from the host
  while (!client.available())
  {
    if (millis() - dataLogTimer > TIMEOUT_PERIOD)
    {
      Serial.println("Reply timeout!");
      replyTimeouts++;
      postingTime = -(millis() - dataLogTimer);
      client.stop();
      return;
    }
  }

  // show reply from the host
  while (client.available())
  {
    Serial.write(client.read());
  }
  Serial.print(" > Done");
  client.stop();

  // show time to log the data
  postingTime = millis() - dataLogTimer;
  Serial.printf(" in %d ms\n", postingTime);
}


void nextSettings(void)
{
  numberOfSamples = numberOfSamples * 1.66 + 0.5;
  if (numberOfSamples > NUMBER_OF_SAMPLES)
  {
    numberOfSamples = 20;
    samplingPause = samplingPause * 1.66 + 0.5;
    if (samplingPause > 110)
    {
      samplingPause = 1;
    }
  }
}


//
// Monitor Wi-Fi connection if it is alive
// If not alive then wait until it reconnects
//
void isWiFiAlive(void)
{
  timeConnected = (millis() - millisReconnected) / 1000;
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Not connected ");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(500);
    }
    Serial.println(" now connected");

    logToThingSpeak();
    nextSettings();

    totalSamples = 0;
    numberOfReconnects++;
    millisReconnected = millis();
    showStatisticsTimer = millisReconnected;
  }

  // log anyway if connected for a longer than 5 minutes
  // 5 mintues = 300000 ms
  if(millis() - millisReconnected > 300000)
  {
    logToThingSpeak();
    nextSettings();

    totalSamples = 0;
    millisReconnected = millis();
    showStatisticsTimer = millisReconnected;
  }
}


void setup(void)
{
  Serial.begin(115200);
  Serial.println("\nEspScopeA0-Bravo 0.2.4");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  Serial.printf("Connecting to %s\n", ssid);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Connected, IP address: ");
  Serial.println(WiFi.localIP());
}


void loop(void)
{
  isWiFiAlive();
  analogSample();
  showStatistics();
}
