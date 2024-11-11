### PREDICTIVE MAINTENANCE IN INDUSTRY AMONG IOT NODES

### The Script for Node1 and Node2 is given -- the Functionalities includes 
Nodes ::
This project is an IoT-based anomaly detection system for environmental monitoring, built using an ESP8266 microcontroller, DHT11 temperature and humidity sensor, MQ3 air quality detection and MQTT protocol. The system reads temperature and humidity data and identifies anomalies by comparing sensor readings to real-time weather data obtained from an online API based on the device’s location. When a deviation is detected, the system uses AES-128 encryption to secure anomaly notifications sent via MQTT.

Key functionalities include:

Anomaly Detection: Temperature and humidity readings are monitored in real-time. If sensor readings deviate significantly from current weather data, anomalies are flagged.
Real-time Location and Weather Integration: The ESP8266 fetches location data based on the device’s IP address, retrieves local weather from the OpenWeatherMap API, and compares it to the anomaly values to verify discrepancies.
Secure Communication: Anomalies are transmitted securely using AES-128 encryption, preventing unauthorized data access and ensuring integrity.
Noisy Data Handling: The system incorporates randomized noise generation for missing or NaN values, allowing robust operation even when sensor data is unavailable.


