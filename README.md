# ESP32 ThingsBoard Modbus Controller

This project implements an IoT gateway using ESP32 to monitor environmental data via Modbus RTU (RS-485) and control devices remotely through the ThingsBoard platform (Localhost) using MQTT.

## Overview

The firmware acts as a bridge between industrial sensors and the IoT cloud. It reads data from a temperature/humidity sensor using the Modbus protocol and publishes telemetry to ThingsBoard. Additionally, it listens for RPC commands from the server to control a fan's speed levels.

## Key Features

* **Modbus Communication:** Reads Input Registers (Temperature, Humidity) via RS485 module using the ModbusMaster library.
* **MQTT Telemetry:** Publishes sensor data and device status to ThingsBoard every 5 seconds.
* **Remote Control (RPC):** Handles `setValue` and `setFanSpeed` commands to adjust fan speed (Levels 1-3).
* **Attribute Synchronization:** automatically updates client attributes (IP Address, Device ID, Supported Levels) upon connection.
* **Robust Connectivity:** Includes logic for automatic WiFi and MQTT reconnection.
* **JSON Parsing:** Utilizes ArduinoJson for efficient and safe data handling.

## Hardware Requirements

* ESP32 Development Board (e.g., ESP32 WROOM).
* RS485 to TTL Module (MAX485).
* Modbus RTU Sensor (Temperature & Humidity).
* Actuator/Fan system.

## Library Dependencies

This project requires the following Arduino libraries:

1.  **WiFi.h**: Built-in ESP32 library.
2.  **PubSubClient**: For MQTT communication.
3.  **ModbusMaster**: For RS485 Modbus RTU protocol.
4.  **ArduinoJson**: For parsing and serializing JSON payloads.

## Configuration

Before flashing, update the credentials in the source code:

* WiFi SSID & Password.
* ThingsBoard Server IP (MQTT Broker).
* Device Access Token.

## Usage Note

The system uses specific topic structures for ThingsBoard:
* Telemetry: `v1/devices/me/telemetry`
* Attributes: `v1/devices/me/attributes`
* RPC Request: `v1/devices/me/rpc/request/+`
