/*
  This application is sampling analog input A0 of ESP8266
  so it then can be displayed on-line in a web browser
  Application consists of several files
  Please check repository below for details

  Repository: https://github.com/krzychb/EspScopeA0
  Version: Charlie
  File: EspScopeA0-Charlie.ino
  Revision: 0.1.2
  Date: 11-Sep-2016
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
// Install WebSocketes library by Markus Sattler
// https://github.com/Links2004/arduinoWebSockets
//
#include <Hash.h>
#include <WebSocketsServer.h>
WebSocketsServer webSocket = WebSocketsServer(81);


// tracking of number of Wi-Fi reconnects
// and connection time
unsigned long numberOfReconnects;
unsigned long millisReconnected;


//
// Continuous sampling rate of A0 in this application is about 10 samples/ms
// Wi-Fi connection gets stuck if continuous A0 sampling is not paused
// Minimum pause duration depends on number of samples, for instance:
// 1000 samples, pause 3ms
// 2000 samples, pause 5ms
// Above values have been established experimentally using this application
// Check them out on your own - they may depend on version of SDK
//
// Ref: https://github.com/esp8266/Arduino/issues/1634
//

#define NUMBER_OF_SAMPLES 5000  // maximum number of samples taken
int samples[NUMBER_OF_SAMPLES];
int numberOfSamples = 200;
unsigned long samplingTime = 1;  // time in us to collect all the numberOfSamples
int samplingPause = 3; // time in ms to pause between continuous sampling

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

//
// process Web Socket events / communication
//
void webSocketEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t lenght)
{
  unsigned long messageNumber;

  switch (type)
  {
    case WStype_DISCONNECTED:
      Serial.printf("[%u] Disconnected!\n", num);
      break;
    case WStype_CONNECTED:
      Serial.printf("[%u] Connected from ", num);
      Serial.println(webSocket.remoteIP(num));
      // send message back to client
      webSocket.sendTXT(num, "Connected");
      break;
    case WStype_TEXT:
      //
      // Format of message to process
      // # MESSAGE_NUMBER
      // other fromats are ignored
      //
      if (payload[0] == '#')
      {
        char* token = strtok((char*) &payload[2], " ");
        messageNumber = (unsigned long) strtol(token, '\0', 10);

        analogSample();

        // send the analog samples back
        String message = "# " + String(messageNumber) + " ";
        for (int i = 0; i < numberOfSamples; i++)
        {
          message = message + String(samples[i]) + ";";
        }
        message[message.length() - 1] = '\0';
        webSocket.sendTXT(num, message);
      }
      else
      {
        Serial.printf("[%u] received Text: %s\n", num, payload);
      }
      break;
    default:
      Serial.println("Case?");
      break;
  }
}


//
// check and process user input
//
void checkUserInput(void)
{
  if (Serial.available() > 0)
  {
    char inChar = Serial.read();
    switch (inChar)
    {
      case 'h':
        Serial.println("h : show this help");
        Serial.println("r/e : reduce/enlarge number of samples by 10");
        Serial.println("d/i : decrease/increase sampling pause by 1ms  ");
        Serial.println("samples : per s : pause ms : reconnects : connected s : avg per s");
        break;
      case 'e':
        numberOfSamples += 5;
        if(numberOfSamples > NUMBER_OF_SAMPLES)
        {
          numberOfSamples = NUMBER_OF_SAMPLES;
        }
        Serial.printf("Number of samples enlarged to %d\n", numberOfSamples);
        break;
      case 'r':
        numberOfSamples -= 5;
        if(numberOfSamples < 5)
        {
          numberOfSamples = 5;
        }
        Serial.printf("Number of samples reduced to %d\n", numberOfSamples);
        break;
        case 'i':
          Serial.printf("Sampling pause increased to %dms\n", ++samplingPause);
          break;
        case 'd':
          samplingPause--;
          if(samplingPause < 1)
          {
            samplingPause = 1;
          }
          Serial.printf("Sampling pause decreased to %dms\n", samplingPause);
          break;
      default:
        Serial.printf("Case? (%c)\n", inChar);
        break;
    }
  }
}


unsigned long showStatisticsTimer;
void showStatistics(void)
{
  // show statistics every 5s (5000ms)
  if(millis() > showStatisticsTimer + 5000)
  {
    showStatisticsTimer = millis();
    if(totalSamples == 0)
    {
      Serial.println("No any samples collected");
      return;
    }
    Serial.print(numberOfSamples);
    Serial.print(" : ");
    // average number of samples per second excluding pause time
    Serial.print(numberOfSamples * 1000000 / samplingTime);
    Serial.print(" 1/s : ");
    Serial.print(samplingPause);
    Serial.print("ms : ");
    Serial.print(numberOfReconnects);
    Serial.print(" : ");
    long timeConnected = (millis() - millisReconnected) / 1000;
    Serial.print(timeConnected);
    Serial.print("s : ");
    // average number of samples per second including pause time
    // since last connected / reconnected to Wi-Fi
    Serial.print(totalSamples / timeConnected);
    Serial.print(" 1/s");
    Serial.println();
  }
}


//
// Monitor Wi-Fi connection if it is alive
// If not alive then wait until it reconnects
//
void isWiFiAlive(void)
{
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.print("Not connected ");
    while (WiFi.status() != WL_CONNECTED)
    {
      Serial.print(".");
      delay(500);
    }
    Serial.printf(" now connected, my IP: %s\n", WiFi.localIP().toString().c_str());

    totalSamples = 0;
    numberOfReconnects++;
    millisReconnected = millis();
    showStatisticsTimer = millisReconnected;
  }
}


void setup(void)
{
  Serial.begin(115200);
  Serial.println("\nEspScopeA0-Charlie 0.1.2");
  Serial.println("Type h for help");

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
  Serial.println("WebSockets started");
}


void loop(void)
{
  isWiFiAlive();
  checkUserInput();
  webSocket.loop();
  showStatistics();
}
