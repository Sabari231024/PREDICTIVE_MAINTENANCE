from flask import Flask, render_template, Response
import paho.mqtt.client as mqtt
import json
import re
import time
from datetime import datetime
from collections import deque
import numpy as np
from flask_mysqldb import MySQL
from config import Config  # Import the Config class
import joblib
app = Flask(__name__)


app.config['MYSQL_HOST'] = Config.MYSQL_HOST
app.config['MYSQL_USER'] = Config.MYSQL_USER
app.config['MYSQL_PASSWORD'] = Config.MYSQL_PASSWORD
app.config['MYSQL_DB'] = Config.MYSQL_DB
app.config['MYSQL_CURSORCLASS'] = Config.MYSQL_CURSORCLASS


from flask_mysqldb import MySQL
from config import Config
app.config.from_object(Config)
mysql = MySQL(app)

def save_data_to_mysql(sensor_data):
    """Save the sensor data to the MySQL database."""
    try:
        with app.app_context():  
            cursor = mysql.connection.cursor()
            query = """
                INSERT INTO iotdata (time, temp1, temp2, humid1, humid2, airquality)
                VALUES (%s, %s, %s, %s, %s, %s)
            """
            data = (
                datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                sensor_data.get("temperature1", None),
                sensor_data.get("temperature2", None),
                sensor_data.get("humidity1", None),
                sensor_data.get("humidity2", None),
                sensor_data.get("air_quality", None),
            )
            cursor.execute(query, data)
            mysql.connection.commit()
            cursor.close()
            print("Data saved to MySQL successfully!")
    except Exception as e:
        print(f"Error saving data to MySQL: {e}")


broker = "192.168.247.122"
topic1 = "sensor/temperature"        
topic2 = "sensor/humidity"           
topic3 = "sensor/temperature1"       
topic4 = "sensor/humidity1"          
topic5 = "sensor/air_quality"        
anomaly_topic = "sensor/anamoly1"
anomaly_topic1 = "sensor/anamoly" 
sensor_data = {"temperature1": None, "humidity1": None, "temperature2": None, "humidity2": None, "air_quality": None}

WINDOW_SIZE = 30
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from base64 import b64encode
import os

# AES encryption function
def encrypt_message(message, key):
    """Encrypt the message using AES encryption with the provided key."""
    # Ensure the key length is 32 bytes (256 bits) for AES-256 encryption
    key = key.encode('utf-8')[:32]
    
    # Generate a random IV (Initialization Vector)
    iv = os.urandom(16)
    
    # Create an AES cipher object with the key and IV
    cipher = Cipher(algorithms.AES(key), modes.CFB8(iv), backend=default_backend())
    encryptor = cipher.encryptor()
    
    # Encrypt the message
    ciphertext = encryptor.update(message.encode('utf-8')) + encryptor.finalize()
    
    # Return the base64 encoded ciphertext and IV to ensure safe transmission
    return b64encode(iv + ciphertext).decode('utf-8')


temperature_window1 = deque(maxlen=WINDOW_SIZE)
humidity_window1 = deque(maxlen=WINDOW_SIZE)
temperature_window2 = deque(maxlen=WINDOW_SIZE)
humidity_window2 = deque(maxlen=WINDOW_SIZE)
air_quality_window = deque(maxlen=WINDOW_SIZE)

Z_SCORE_THRESHOLD = 3.6
VALUE_DIFF_THRESHOLD = 3.0  

def is_value_close(sensor_value, other_node_window):
    """Check if the sensor value is close to the most recent value in the other node's deque."""
    if len(other_node_window) == 0:
        return False
    
    recent_value = other_node_window[-1]
    return abs(sensor_value - recent_value) <= VALUE_DIFF_THRESHOLD
def publish_anomaly(client, value, topic_name):
    """Publish the anomaly message to the appropriate anomaly topic."""
    message1 = ""
    target_topic = anomaly_topic   

    if topic_name == 'temp1':
        message1 = f"[temp][{value}]node1"
        print(f"PUBLISHING ||||{message1}")
    elif topic_name == 'humidity1':
        message1 = f"[humd][{value}]node1"
        print(f"PUBLISHING ||||{message1}")
    elif topic_name == 'temp2':
        message1 = f"[temp][{value}]node2"
        target_topic = anomaly_topic1  
        print(f"PUBLISHING ||||{message1}")
    elif topic_name == 'humidity2':
        message1 = f"[humd][{value}]node2"
        target_topic = anomaly_topic1  
        print(f"PUBLISHING ||||{message1}")
    elif topic_name == 'air_quality':
        message1 = f"[aq][{value}]node1"

    if message1:
        # Encrypt the message before publishing
        #message1 = encrypt_message(message1, "hellosnuapplycyber")
        print(f"Publishing encrypted anomaly on {target_topic}: {message1}")
        mqtt_client.publish(target_topic, message1)



def anomaly_detection_rolling(sensor_value, window, topic_name):
    """Detect anomalies using rolling mean and standard deviation."""
    window.append(sensor_value)
    if len(window) == WINDOW_SIZE:
        mean = np.mean(window)
        std_dev = np.std(window)
        z_score = (sensor_value - mean) / std_dev
        print(f"Z-score for {topic_name}: {z_score} (mean: {mean}, std_dev: {std_dev})")
        if abs(z_score) > Z_SCORE_THRESHOLD:
            print(f"Anomaly detected for {topic_name} at value {sensor_value}")
            if topic_name == 'temp1':
                if not is_value_close(sensor_value, temperature_window2):
                    publish_anomaly(client, sensor_value, topic_name)
            elif topic_name == 'humidity1':
                if not is_value_close(sensor_value, humidity_window2):
                    publish_anomaly(client, sensor_value, topic_name)
            elif topic_name == 'temp2':
                if not is_value_close(sensor_value, temperature_window1):
                    publish_anomaly(client, sensor_value, topic_name)
            elif topic_name == 'humidity2':
                if not is_value_close(sensor_value, humidity_window1):
                    publish_anomaly(client, sensor_value, topic_name)
            else:
                publish_anomaly(client, sensor_value, topic_name)

def on_message(client, userdata, message):
    global sensor_data
    try:
        payload = message.payload.decode('utf-8')
        print(f"Received message from {message.topic}: {payload}")
        match = re.search(r"[-+]?\d*\.\d+|\d+", payload)
        if match:
            value = float(match.group())
            if message.topic == topic1:
                sensor_data["temperature1"] = value
                anomaly_detection_rolling(value, temperature_window1, "temp1")
            elif message.topic == topic2:
                sensor_data["humidity1"] = value
                anomaly_detection_rolling(value, humidity_window1, "humidity1")
            elif message.topic == topic3:
                sensor_data["temperature2"] = value
                anomaly_detection_rolling(value, temperature_window2, "temp2")
            elif message.topic == topic4:
                sensor_data["humidity2"] = value
                anomaly_detection_rolling(value, humidity_window2, "humidity2")
            elif message.topic == topic5:
                sensor_data["air_quality"] = value
                anomaly_detection_rolling(value, air_quality_window, "air_quality")
            
            save_data_to_mysql(sensor_data)
    except Exception as e:
        print(f"Error processing message: {e}")

# MQTT Setup
mqtt_client = mqtt.Client()
mqtt_client.on_message = on_message
mqtt_client.connect(broker, 1883)
mqtt_client.subscribe([(topic1, 0), (topic2, 0), (topic3, 0), (topic4, 0), (topic5, 0)])
mqtt_client.loop_start()

client = mqtt.Client()
client.connect(broker, 1883)

@app.route('/')
def index():
    return render_template('index.html')

@app.route('/chart-data')
def chart_data():
    def event_stream():
        predicted_air_quality = None
        
        while True:
            # Load the ARIMA model for air quality prediction
            try:
                air_quality_model_path = 'arima_models/arima_model_airquality.joblib'
                air_quality_model = joblib.load(air_quality_model_path)

                # Forecast next value for air quality if the window is full
                if len(air_quality_window) == WINDOW_SIZE:
                    # Convert deque to numpy array for ARIMA input
                    air_quality_input_data = np.array(air_quality_window)

                    # Forecast next value for air quality
                    predicted_air_quality = air_quality_model.forecast(steps=1, exog=air_quality_input_data[-WINDOW_SIZE:])[0]
                else:
                    predicted_air_quality = None
            except Exception as e:
                print(f"Error in ARIMA prediction for air quality: {e}")
                predicted_air_quality = None

            # Prepare the JSON data with both real-time and predicted values
            json_data = json.dumps({
                "time": datetime.now().strftime('%H:%M:%S'),
                "temperature1": sensor_data.get("temperature1", 0),
                "humidity1": sensor_data.get("humidity1", 0),
                "temperature2": sensor_data.get("temperature2", 0),
                "humidity2": sensor_data.get("humidity2", 0),
                "air_quality": sensor_data.get("air_quality", 0),
                "predicted_air_quality": predicted_air_quality if predicted_air_quality is not None else 0
            })

            # Send the data as Server-Sent Events (SSE)
            yield f"data: {json_data}\n\n"
            time.sleep(1)  # Adjust this to the desired data update frequency
        
    return Response(event_stream(), mimetype="text/event-stream")


if __name__ == '__main__':
    app.run(debug=True, host='0.0.0.0', port=5000)
