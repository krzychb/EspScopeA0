## Alfa

This application is sampling analog input A0 of ESP8266. It also helps to check influence of A0 sampling on Wi-Fi connection. According to issue report [#1634](https://github.com/esp8266/Arduino/issues/1634) continuous sampling of analog input causes Wi-Fi to disconnect.

Features:

* Sampling of A0 at maximum speed
* Adjustable number of samples
* Adjustable sampling pause
* Presentation of sampling rates and Wi-Fi connection time


## Table of Contents

* [Installation](#installation)
* [Usage](#usage)
* [Functionality](#functionality)
  * [Is Wi-Fi Alive?](#is-wi-fi-alive)
  * [Check User Input](#check-user-input)
  * [Analog Sample](#analog-sample)
  * [Show Statistics](#show-statistics)
* [Compatibility](#compatibility)
  * [Hardware](#hardware)
  * [Arduino Core](#arduino-core)
  * [Programming IDEs](#programming-ides)


## Installation

Update SSID and password in [EspScopeA0-Alfa.ino](EspScopeA0-Alfa/EspScopeA0-Alfa.ino) sketch so the module can join your Wi-Fi network.

Upload updated sketch to ESP module and open a serial monitor.


## Usage

After reset or power up the module will display the following message:

```
EspScopeA0-Alfa 0.1.1
Type h for help
Not connected ........ now connected
1000 : 10556 1/s : 3ms : 1 : 5s : 10400 1/s
1000 : 10553 1/s : 3ms : 1 : 10s : 10400 1/s
1000 : 10553 1/s : 3ms : 1 : 15s : 10400 1/s
```

After pressing `h` you should see available program options displayed.

```
h : show this help
r/e : reduce/enlarge number of samples by 10
d/i : decrease/increase sampling pause by 1ms
samples : per s : pause ms : reconnects : connected s : avg per s
1000 : 10551 1/s : 3ms : 1 : 60s : 10400 1/s
```

The last line of help contains headers to sampling statistics:

* `samples` - number of samples collected in one run
* `per s` - sampling rate (samples / second)
* `pause ms` - pause in milliseconds between sampling runs
* `reconnects` - number of Wi-Fi reconnects
* `connected s` - time in seconds since last reconnect
* `avg per s` - average sampling rate (samples / second) including pauses and time to reconnect

Sampling is done continuously without any delays in a loop to collect set number of `samples`. After completion application will wait set number of milliseconds `pause ms` and then start sampling again.

Using the keyboard you can change the number of samples or duration of pause between sampling. With `r` you can reduce and with `e` enlarge the number of samples. Keys `d` and `i` are used to decrease or increase the sampling pause.

For large number of samples and short delays between sampling runs, Wi-Fi will randomly disconnect. This is likely due to interference with RF calibration routines done in background by ESP using the same ADC.

Disconnects will be reported on serial monitor by showing dots `...` while module is reconnecting and by incrementing the counter marked as `reconnects`.

```
1000 : 10554 1/s : 3ms : 3 : 343s : 10309 1/s
1000 : 10557 1/s : 3ms : 3 : 348s : 10310 1/s
1000 : 10551 1/s : 3ms : 3 : 353s : 10311 1/s
Not connected ............. now connected
1000 : 10553 1/s : 3ms : 4 : 5s : 10400 1/s
1000 : 10552 1/s : 3ms : 4 : 10s : 10400 1/s
Not connected ........ now connected
1000 : 10558 1/s : 3ms : 5 : 5s : 10400 1/s
1000 : 10553 1/s : 3ms : 5 : 10s : 10400 1/s
```

Sampling rate measured in above test is slightly above 10 kHz and causing Wi-Fi to disconnect. As for comparison the following rates are quoted in [Espressif ESP8266 FAQs](https://espressif.com/sites/default/files/documentation/esp8266_faq_en.pdf) of 2016.08 on page 18/32:

* 100k samples/sec (Wi-Fi modem turned off)
* 1k samples/sec (Wi-Fi modem normally active)

Do couple of tests changing the number of samples and pause. Gauge at what values module starts disconnecting from Wi-Fi network. This will give you an idea how many samples you will be able to collect, and how often without breaking the Wi-Fi connection. After all you need the Wi-Fi operational to send the samples out for on-line visualization in a web browser.

If you do not like experiments / testing and prefer to be on the safe side, just follow 1k samples/sec figure specified by Espressif FAQ.



## Functionality


Application starts off in `setup()` by initializing serial interface and by configuring Wi-Fi connection. Module is configured in station mode. 

```cpp
void setup(void)
{
  Serial.begin(115200);
  Serial.println("\nEspScopeA0-Alfa 0.1.1");
  Serial.println("Type h for help");
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
}
```

Then in the `loop()` the following non-blocking functions are executed:

```cpp
void loop(void)
{
  isWiFiAlive();
  checkUserInput();
  analogSample();
  showStatistics();
}
```

Below is description of functions in order they are executed.


### Is Wi-Fi Alive?

Monitor Wi-Fi connection if it is alive - `if (WiFi.status() != WL_CONNECTED)`. If it is not alive, then wait until it reconnects  - `while (WiFi.status() != WL_CONNECTED)`.

```cpp
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
    Serial.println(" now connected");
    totalSamples = 0;
    numberOfReconnects++;
    millisReconnected = millis();
    showStatisticsTimer = millisReconnected;
  }
}
```


### Check User Input

Function is checking serial input `if (Serial.available() > 0)` and reading characters entered `char inChar = Serial.read()` on keyboard. In response to specific character entered function will change number of samples, sampling pause and display help.

```cpp
void checkUserInput(void)
{
  if (Serial.available() > 0)
  {
    char inChar = Serial.read();
    switch (inChar)
    {
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
        
     (...)

      default:
        Serial.printf("Case? (%c)\n", inChar);
        break;
    }
  }
}
```


### Analog Sample

This is the key function of this sketch, responsible for sampling of the analog input. New sampling starts only after the `samplingPause` expires. This is checked by condition `if  (millis() < millisLastSample + samplingPause)`. Once started, function will collect `numberOfSamples` and save them in the table `samples[]`. Variable `samplingTime` is used to calculate the sampling rate (number of samples per second).


```cpp
void analogSample(void)
{
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
```


### Show Statistics

The last function to run in the `loop()` is called `showStatistics()` and responsible for printing out sampling statistic with interval of 5 seconds.


## Compatibility

[EspScopeA0-Alfa](EspScopeA0-Alfa/EspScopeA0-Alfa.ino) has been successfully tested with the following h/w and s/w.


#### Hardware

* NodeMCU 1.0 (ESP-12E Module) @ 80MHz, 1MB SPIFFS


#### Arduino Core

* [Esp8266 / Arduino](https://github.com/esp8266/Arduino) core [2.3.0](https://github.com/esp8266/Arduino/releases/tag/2.3.0) for Arduino IDE and Visual Micro
* [framework-arduinoespressif](http://platformio.org/platforms/espressif) version 13 for PlatformIO


#### Programming IDEs

* [Arduino IDE](https://www.arduino.cc/en/Main/Software) 1.6.9 portable version running on Windows 7 x64
* [PlatformIO IDE](http://platformio.org/platformio-ide) 1.4.0 CLI 2.11.2 running on Windows 7 x64
* [Visual Micro](http://www.visualmicro.com/) 1606.17.10 with Visual Studio Community 2015 running on Windows 7 x64

