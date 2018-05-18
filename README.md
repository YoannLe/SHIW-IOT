# SmartHome-In-Wall-IOT

Smart Home in-wall IOT modules with ESP8266 designed to be used with Arduino and REST API or Souliss, OpenHab,...

Goals and prospects:
* Modules must fit in standard electric wall boxes (European standard wallbox ⌀67x40mm)
* Limit the number of components for DIY feasibility
* Common logic board and different power boards

This GitHub contains PCB files and source code samples.

##Hardware
Schematic and PCB:
* Logic Board:
 * Made for ESP-07 or EPS-12 ESP8266 module
 * Drives all power boards
 * Include relay driver, I2C port for Temperatur and Humidity sensor
* Power Boards:
 * Relays version: Relays Board, 2 dual coils relays, for curtains, garage doors...
 * FilPilote version: Generate signals for drive Electric Heaters by Fil Pilote
 * Dimmer version: Triac with zero crossing circuit to dim incandescent lamps

##Software
Arduino source code sample for each module, can works out of the box but understanding Souliss architecture and configration is required to build a Smart Home Network with a lot of nodes.

##Documentation



##References
[Arduino and ESP8266 GitHub](https://github.com/esp8266/Arduino)

[Souliss Wiki](https://github.com/souliss/souliss/wiki) 

[Souliss Website](http://souliss.net/)

[Fil Pilote] (http://www.radiateur-electrique.org/fil-pilote-radiateur.php)

# License
