# Automated-Pet-Feeder

This project implements an automated pet feeder using the TM123GH6PM Tiva board. The system dispenses pet food at scheduled times and water based on owner's settings.

## Features

- **Scheduled Feeding:** Set up specific feeding times for your pet.
- **Water Regulation Option:** Dispense water based on low water or motion detection.
- **Alert Mode Option:** Alert the owner if the water source is low. 

## Hardware Components
  |                         |
  | ------------------------|
  | TM123GH6PM Tiva Board   |
  |  Passive IR Sensor      |
  | Speaker                 |
  | 12V Motors connected to MOSFETS (x2)|
  | Diodes                       |
  | Capacitative Water Sensing circuit   |

### Circuit Diagrams

<p align="center">
<img src="Documentation/Auger and Motor Circuit.png">
<p align="center"> Circuit used for the Auger and Water Dispenser. </p>
</p>

<p align="center">
<img src="Documentation/Capacitative Water Sensing Circuit.png">
<p align="center"> Circuit used for capacitative bowl. </p>
</p>

<p align="center">
<img src=Documentation/Speaker Circuit.png">
<p align="center"> Circuit used for speaker. </p>
</p>
 
## Peripherals Used
  |                         |
  | ------------------------|
  | GPIO   |
  |  UART  |
  | HIB    |
  | EEPROM |
  |  Timers |
  |   PWM  |
  | Analog Comparator |

## Software FEatures
- 

## Interface

User can use a serial interface (Putty) to input feeding schedules and change other settings
<p align = center>
<img src = "Documentation/Interface.png" width="250" >
</p>
