### PREDICTIVE MAINTENANCE IN INDUSTRY AMONG IOT NODES

The Script for Node1 and Node2 is given -- the Functionalities includes 

### Nodes ::
This project is an IoT-based anomaly detection system for environmental monitoring, built using an ESP8266 microcontroller, DHT11 temperature and humidity sensor, MQ3 air quality detection and MQTT protocol. The system reads temperature and humidity data and identifies anomalies by comparing sensor readings to real-time weather data obtained from an online API based on the device’s location. When a deviation is detected, the system uses AES-128 encryption to secure anomaly notifications sent via MQTT.

Key functionalities include:

* Anomaly Detection: Temperature and humidity readings are monitored in real-time. If sensor readings deviate significantly from current weather data, anomalies are flagged.
* Real-time Location and Weather Integration: The ESP8266 fetches location data based on the device’s IP address, retrieves local weather from the OpenWeatherMap API, and compares it to the anomaly values to verify discrepancies.
* Secure Communication: Anomalies are transmitted securely using AES-128 encryption, preventing unauthorized data access and ensuring integrity.
* Noisy Data Handling: The system incorporates randomized noise generation for missing or NaN values, allowing robust operation even when sensor data is unavailable.

The Script for app.py is given -- the Functionalities includes 

Note : Inorder to connect an both Backend server and Nodes and Mosquitto Based Broker must be Running on a different server.


### App.py ::
This Python Flask application is designed for real-time IoT data monitoring, anomaly detection, and predictive analytics. It leverages MQTT for data ingestion from multiple sensors and integrates MySQL for storage. Key functions include:

* Data Acquisition and Storage: Sensor data is received through MQTT topics, which include temperature, humidity, and air quality metrics from multiple nodes. Each message is parsed, and the data is saved to a MySQL database.

* Anomaly Detection: Using a rolling Z-score method, sensor values are compared against thresholds to detect anomalies. Anomalies trigger MQTT messages to be published to specific topics for monitoring purposes.

* AES Encryption: Anomalous data messages are encrypted using AES encryption before being published, enhancing the security of the transmitted data.

* Predictive Analytics: An ARIMA model is used to forecast air quality based on historical data stored in a rolling window. Predicted values are included in the real-time data stream for monitoring future air quality trends.

* Data Visualization: The application includes a simple real-time dashboard, updated using Server-Sent Events (SSE). This dashboard displays both the actual sensor values and predicted air quality, allowing for real-time insights and anomaly detection.

### Prediction --

ARIMA (an time series model ) MODEL IS USED TO PREDICT THE DATA.

This script preprocesses time-series sensor data, fills missing values, and applies ARIMA forecasting for temperature, humidity, and air quality metrics. It involves the following steps:

* Data Imputation: The function fill_nan_with_next_mean scans each column for missing values, replacing each NaN with the mean of the next available 100 non-missing values. The data is processed in chunks, optimizing memory usage, and is then saved to a CSV file.

* Train-Test Split: After cleaning, the data is split into training and testing sets, with an 80-20 split to allow for model evaluation.

* Stationarity Testing and Differencing: For each feature, an Augmented Dickey-Fuller (ADF) test is performed to check for stationarity. If non-stationarity is detected, differencing is applied to stabilize the series.

* ARIMA Modeling and Forecasting: An ARIMA model is trained on each feature’s training data, with hyperparameters selected automatically. The model then forecasts the test data points, comparing them with the actual values.

* Evaluation and Visualization: Forecast accuracy is assessed using Mean Squared Error (MSE) and Mean Absolute Error (MAE). The results, including both actual and forecasted values, are visualized in line plots for each feature, showing model performance and highlighting potential trends.

### Config.py 

Configuration file to ocnnect to MySql

---------------------------------------------------
The Trained Models are stored in arima_models 
Project done by [Sabarisrinivas004@gmail.com](mailto:sabarisrinivas004@gmail.com)


