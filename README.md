![Logo](pics/Clock.jpg)
# LIXIE Clock with DCF77 encoder

## Description
This project is a full source to create a LIXIE clock with DCF77 support and Webpage to configure the clock. 

## Features
- the code creates a filesystem on flash storage of the esp8266
- all settings are stored on the filesystem in a JSON format
- Wifi-Manager for easy connection to available AccessPoints
- Webpage to configure all settings or read the values
- OTA Over-The-Air update of firmware
- Color picker for all color states

## ToDo
- Additional sensors like BMP280, BME280, SHT3x
- Pushbuttons to configure the clock
- Alarm mode

## Schematic

| NodeMCU Board Pins |     | Device Pin         | Device Name  |   
|--------------------|-----|--------------------|--------------|      
| GPIO3              | RX  | Data in            | WS2812B LED  |   
| Vin or 5V          | Vin | +5V                | WS2812B LED  |
| GND                | GND | GND                | WS2812B LED  |
| GPIO13             | D7  | DCF77 signal       | DCF77        |
| GND                | GND | GND                | DCF77        |
| VDD 3.3V           | VDD | DCF77 Vss          | DCF77        |

![Logo](pics/Lasercut.jpg)
![Logo](pics/Number.jpg)

## Weblinks to get running
- http://www.kidbuild.de or https://shop.kidbuild.de
E-Mail info@kidbuild.de

## Changelog 

### Version 0.1
20231013
- Initial Version
- DCF77 support