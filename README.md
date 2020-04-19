# pulsie
Count and transmit number of pulses detected on digital input of a NodeMCU.
## Typical usage
Keep track of household water usage by "listening" to an analogue water counter using a reed switch.
## Objectives
* Set up a WIFI connection to an access point.
* Monitor the status of a digital input port. 
* Debounce the digital input port by adding a debounce timer to the logic.
* Keep track of the number of pulses detected on the input port.
* Send the number of pulses - counted during the latest monitoring period - to an InfluxDB server.
* Use the onboard LEDs to indicate the WIFI connection status, detection of pulses and the upload to InfluxDB server.

## To do
* Check if NodeMCU is by default programmed to enter a light sleep mode, possibly causing it to miss the first set of pulses after a long "pulse-less" period of time. 
   
   [ESP8266 Low Power Solutions](https://www.espressif.com/sites/default/files/9b-esp8266-low_power_solutions_en_0.pdf)

* Check how to write rollover-safe code.

   [How can I handle the millis() rollover?](https://arduino.stackexchange.com/questions/12587/how-can-i-handle-the-millis-rollover)

## Inspiration
* [Arduino Object Oriented Programming](https://roboticsbackend.com/arduino-object-oriented-programming-oop/)

## Learned/discoverd by working on this little project
* [Code style online checker](http://cpplint.appspot.com/#)
* [How to customize C++’s coding style in VSCode](https://medium.com/@zamhuang/vscode-how-to-customize-c-s-coding-style-in-vscode-ad16d87e93bf)