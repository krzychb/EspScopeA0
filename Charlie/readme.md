## Charlie :construction:

This application is sampling analog input A0 of ESP8266, so it then can be displayed on-line in a web browser.

Features:

* Sampling of A0 at maximum speed
* Adjustable number of samples
* Adjustable sampling pause
* Visualization of samples in web browser
* Data transfer for visualization with web sockets

If possible connect a variable signal source to A0 to have some wave-forms to observe. Ideally this should be a signal generator with adjustable frequency and output voltage. Another option is connecting a microphone with an amplifier. The signal frequency of human voice is perfectly fine, the signal range should be within 0 - 1V. Please note, that same ESP modules like NodeMCU have a voltage divider on analog input. In such case you may need to adjust the signal range of source to obtain 0 - 1V on analog input pin A0 (TOUT).

In case you don't have any signal generator or a microphone then you can still do some testing by touching the A0 pin with your finger. This will be measured by ESP8266 as some random nose, unless you are using a wrist strap connected to ground the same ground as the power supply of the module.