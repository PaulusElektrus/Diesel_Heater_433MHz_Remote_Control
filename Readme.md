# Diesel Heater 433 MHz Remote Control

This project is based on [ChilliChump`s Project](https://github.com/ChilliChump/Diesel-Heater-433mhz-Remote-Control). 

Its goal is to control a diesel heater to be a frost guard.
It is done with an Arduino, DHT22 (temperature sensor) and a 433MHz Transmitter.
We sniff and use the protocoll of the original remote control of the heater.

## Projectplan / To-do:

- [X] Basic 433MHz Send & Receive Test
- [X] Sniff Remote Control Codes
- [X] DHT22 integration & test
- [ ] Programming
- [X] Design Circuit Board 
- [ ] Build Circuit Board

## Basic 433Mhz Send & Receive Test

You can install the [rc-switch library](https://docs.arduino.cc/libraries/rc-switch/) in the library manager of the [Arduino IDE](https://www.arduino.cc/en/software). 

Then the ReceiveDemo_Advanced and the SendDemo can be found under "Examples" in the IDE. Just load it on two separate Microcontrollers and open the serial monitor on the receiving one with baudrate 9600. You should now see the received data which was sent from the transmitting Microcontroller.

## Sniffed Remote Control Codes

I used [the receive demo advanced example script](https://github.com/sui77/rc-switch/tree/master/examples/ReceiveDemo_Advanced) to sniff the remote control codes:

### Chinese Diesel Heater Protocol

#### On:
Decimal: 2794262960 (32Bit) 

Binary: 10100110100011010001000110110000 

Tri-State: not applicable 

PulseLength: 257 microseconds 

Protocol: 1

Raw data: 8016,724,276,236,776,724,276,236,776,236,772,728,272,724,280,236,772,724,280,232,776,236,776,232,776,720,280,724,284,224,780,720,276,232,768,240,772,240,772,724,276,236,772,240,776,232,776,724,276,724,276,236,776,728,272,728,276,232,776,236,776,236,772,236,768,

#### Off:
Decimal: 2794259720 (32Bit) 

Binary: 10100110100011010000010100001000 

Tri-State: not applicable 

PulseLength: 257 microseconds 

Protocol: 1

Raw data: 8004,724,272,244,768,728,268,240,772,240,768,728,272,728,268,244,768,724,276,236,772,236,772,236,772,728,272,724,276,236,772,724,276,236,776,232,772,240,768,240,772,236,772,724,276,236,772,724,276,240,768,236,772,236,768,240,772,724,272,244,768,240,768,240,764,

#### +:
Decimal: 2794260720 (32Bit) 

Binary: 10100110100011010000100011110000 

Tri-State: not applicable 

PulseLength: 257 microseconds 

Protocol: 1

Raw data: 8008,720,284,228,780,716,284,228,780,228,780,716,284,716,284,224,784,716,284,228,780,228,780,224,788,712,284,716,288,220,784,716,280,232,776,232,776,228,780,236,772,724,280,228,780,228,780,228,780,720,280,720,280,720,280,716,284,228,780,232,780,228,776,232,776,

#### -:
Decimal: 2794259080 (32Bit) 

Binary: 10100110100011010000001010001000 

Tri-State: not applicable 

PulseLength: 256 microseconds 

Protocol: 1

Raw data: 8008,716,284,232,772,728,276,228,780,232,780,716,280,720,280,224,784,716,280,228,784,220,784,228,784,716,284,720,280,224,784,716,280,228,780,228,780,236,776,224,780,232,780,224,784,712,284,232,780,716,280,232,780,232,772,236,776,720,280,228,780,228,780,228,780,

## DHT22 Test

The DHT22 can be tested with the [Unified Sensor Example](https://github.com/adafruit/DHT-sensor-library/tree/master/examples/DHT_Unified_Sensor). 

You can install the [DHT sensor library](https://docs.arduino.cc/libraries/dht-sensor-library/) in the library manager. Then the DHT_Unified_Sensor example can be found under "Examples" in the IDE. Just load in on the Microcontroller and open the serial monitor with baudrate 9600. You should see actual temperature and humidity in the serial monitor.

## Schematic

![Schematic](./Schematic.png)
