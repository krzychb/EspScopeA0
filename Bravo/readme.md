## Bravo

This application is sampling analog input A0 of ESP8266 at maximum speed with periodic pauses to test influence of A0 sampling on Wi-Fi connection. According to issue report [#1634](https://github.com/esp8266/Arduino/issues/1634) continuous sampling of analog input causes Wi-Fi to disconnect. Test results are saved to [ThinkSpeak](https://thingspeak.com/) for analysis. 

Features:

* Sampling of A0 at maximum speed with periodic pauses
* Sampling period and pause are changed in a loop by the application
* Monitoring of Wi-Fi connection status
* Saving of test parameters and results to ThingSpeak


## Table of Contents

* [Introduction](#introduction)
* [Functionality](#functionality)
  * [Parameter Setting](#parameter-setting)
  * [Test Duration](#test-duration)
  * [Data Logging](#data-logging)
  * [Data Visualization](#data-visualization)
* [Installation](#installation)
* [Usage](#usage)
* [Results](#results)
* [Compatibility](#compatibility)
  * [Hardware](#hardware)
  * [Arduino Core](#arduino-core)
  * [Programming IDEs](#programming-ides)


## Introduction

In previous section [Alfa](../Alfa) I have presented a sketch [EspScopeA0-Alfa.ino](../Alfa/EspScopeA0-Alfa/EspScopeA0-Alfa.ino) that is sampling A0 of ESP8266. With this sketch your are able to collect specific number of A0 samples, make a pause and sample again. All in continuous infinite loop with possibility to adjust the number of samples and the sampling pause on the fly. If sampling is done to often then Wi-Fi connection is lost. This happens due to mistreatment of internal ADC that, besides reading A0, is also used to calibrate Wi-Fi modem. By adjusting the number of samples and sampling pause we can find out the critical vales of both parameters that make the Wi-Fi to disconnect.

After doing coupe of tests I have found out that discovering the critical values of both parameters takes time. Wi-Fi connection may get lost after pretty random period for the same parameter values. Instead of tediously adjusting the values and observing Wi-Fi status, I decided to make all the work by the sketch.


## Functionality

Application has been prepared by copying [EspScopeA0-Alfa.ino](../Alfa/EspScopeA0-Alfa/EspScopeA0-Alfa.ino) and introducing the following changes:

  1. Procedure of checking for user input `checkUserInput()` has been removed
  2. Instead, sampling parameters are changed by function `nextSettings()`
  3. This is done by updated function `isWiFiAlive(void)` on each connection loss or test period expiration
  4. New function `logToThingSpeak()` has been added to log the data in the cloud for visualization and  analysis

More details on particular changes are provided below.


### Parameter Setting

:construction:

```
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
```


### Test Duration

:construction:


### Data Logging

:construction:

| ThingSpeak Field | ThingSpeak Name | Sketch Variable or Function Name | Description |
| --- | --- | --- | --- |
| Field 1 | Run Time [min] | `dataLogTimer / 60000` | Total time in minutes since last reset or power up of module |
| Field 2 | Free Heap [Bytes] | `system_get_free_heap_size()` | Free cheap size in bytes reported by module |
| Field 3 | Connection Failures | `connectionFailures` | Number of times module failed to connect to ThingSpeak |
| Field 4 | Number of Reconnects | `numberOfReconnects` | Number of Wi-Fi reconnects |
| Field 5 | Time Connected [s] | `(millis() - millisReconnected) / 1000` | Time in seconds during test duration that module was connected to Wi-Fi |
| Field 6 | Total Samples | `totalSamples` | Total number A0 samples collected by module since last reset or power up |
| Field 7 | Sampling Pause [ms] | `samplingPause` | Pause in milliseconds after each continuous sampling of A0 |
| Field 8 | Number of Samples | `numberOfSamples` | Number of samples taken during continuous sampling of A0 |


### Data Visualization

:construction:

https://thingspeak.com/channels/153983


## Installation

Update SSID and password in [EspScopeA0-Alfa.ino](EspScopeA0-Alfa/EspScopeA0-Alfa.ino) sketch so the module can join your Wi-Fi network.


Upload updated sketch to ESP module and open a serial monitor.


## Usage

After reset or power up the module will display the following message:

:construction:


## Results

:construction:


## Compatibility

[EspScopeA0-Bravo](EspScopeA0-Bravo/EspScopeA0-Bravo.ino) has been successfully tested with the following h/w and s/w.


#### Hardware

* NodeMCU 1.0 (ESP-12E Module) @ 80MHz, 1MB SPIFFS


#### Arduino Core

* [Esp8266 / Arduino](https://github.com/esp8266/Arduino) core [2.3.0](https://github.com/esp8266/Arduino/releases/tag/2.3.0) for Arduino IDE and Visual Micro
* [framework-arduinoespressif](http://platformio.org/platforms/espressif) version 13 for PlatformIO


#### Programming IDEs

* [Arduino IDE](https://www.arduino.cc/en/Main/Software) 1.6.11 portable version running on Windows 7 x64
* [PlatformIO IDE](http://platformio.org/platformio-ide) 1.4.0 CLI 2.11.2 running on Windows 7 x64
* [Visual Micro](http://www.visualmicro.com/) 1606.17.10 with Visual Studio Community 2015 running on Windows 7 x64

