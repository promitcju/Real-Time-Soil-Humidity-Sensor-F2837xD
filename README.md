# Real-Time Soil Humidity Estimator using TMS320F2837xD

## Project Overview

An embedded real-time soil humidity monitoring system developed using the Texas Instruments TMS320F2837xD microcontroller.

The system acquires an analog sensor signal using the ADC, processes the measurement to estimate soil moisture, and provides real-time visual and audio feedback through an I2C LCD, SPI TFT display, LED, and buzzer.

## Key Features

- Real-time analog sensor acquisition using ADC
- ADC signal filtering
- Soil moisture percentage estimation
- CPU Timer interrupt
- PWM-based LED control
- PWM-based buzzer control
- SPI communication with ILI9341 TFT display
- I2C communication with 16x2 LCD
- Dynamic pixel-based visualization of moisture level
- GPIO configuration and peripheral control

## Hardware

- TI TMS320F2837xD
- Soil moisture sensor
- ILI9341 TFT LCD
- 16x2 I2C LCD
- LED
- Buzzer

## Peripherals Used

| Peripheral | Application |
|---|---|
| ADC | Soil moisture sensor measurement |
| SPI | ILI9341 TFT display |
| I2C | 16x2 LCD |
| ePWM | LED and buzzer control |
| CPU Timer | Periodic background update |
| GPIO | Peripheral control signals |

## Moisture-Level Visualization

The measured moisture level determines the visualization state on the TFT display:

| Moisture Level | Visualization |
|---|---|
| 0–40% | OFF |
| >40–60% | Small |
| 60–80% | Medium |
| ≥80% | Large |

## Signal Processing

The ADC measurement is filtered using an exponential moving filter before conversion into a moisture percentage.

The processed measurement is then used to control the display and PWM outputs.

## Software

- Embedded C
- TI C2000 architecture
- ADC programming
- SPI communication
- I2C communication
- PWM generation
- Interrupt programming
- GPIO configuration

## Repository Contents

The repository contains the embedded C source code implementing the system initialization, ADC acquisition, moisture calculation, LCD communication, SPI TFT control, PWM generation, and timer interrupt functionality.

## Author

**Promit Chakraborty**

M.Tech – Instrumentation Systems  
Indian Institute of Science (IISc), Bangalore
