# Embedded-Systmes
Embedded systems class project: a security system that monitors a tilt sensor and drives a lock actuator. I built the tilt module and the actuator integration.

Embedded Security System

This project was developed as part of an Embedded Systems course, where the goal was to design and implement a basic security system integrating sensors and actuators on a microcontroller platform.

Overview

The system monitors motion or tilt through an analog tilt sensor and triggers a lock actuator when movement is detected beyond a defined threshold. The project demonstrates key embedded concepts such as sensor integration, ADC sampling, signal filtering, and actuator control.

I specifically worked on:

Implementing the tilt sensor module (reading, calibration, and data filtering)

Integrating the tilt sensor output with the lock actuator logic and control signal

System Components

Microcontroller: Seeeduino XIAO ESP32-C3

Sensor: Murata SCA121T-D03 dual-axis analog inclinometer

Actuator: Electronic lock mechanism (5V)

Supporting files:

tilt_sensor.ino — main Arduino sketch for sensor reading and actuator control

scenario1_config.json — configuration file for system parameters

tilt_sensor_uml.png — UML diagram showing module relationships

tilt_sensor_wiring.png — hardware wiring diagram
