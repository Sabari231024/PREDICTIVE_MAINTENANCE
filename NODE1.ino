#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <DHT.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <AESLib.h>  // Include the AESLib library
#include <Base64.h>
// 16-byte AES key (from the provided key "hellosnuapplycyber")
// 16-byte AES key (from the provided key "hellosnuapplycyber")
// The 16-byte key for AES-128 encryption (same as the key used in Python)


#define DHTPIN D1     // Pin where the DHT11 sensor is connected (D1 is GPIO5)
#define DHTTYPE DHT11 // DHT11 sensor

// LED Pin
#define LED_PIN D2    // Pin where the LED is connected

// Replace with your network credentials and broker details
const char* ssid = "sabbu";           // Your WiFi SSID
const char* password = "sabari231024"; // Your WiFi password
const char* mqtt_server = "192.168.1.105"; // MQTT broker IP address

// MQTT topics
const char* topic1 = "sensor/temperature";
const char* topic2 = "sensor/humidity";
const char* topic_anomaly = "sensor/anamoly1";  // Topic for anomaly detection

// Initialize DHT sensor
DHT dht(DHTPIN, DHTTYPE);

// Initialize WiFi and MQTT clients
WiFiClient espClient;
PubSubClient client(espClient);

// Flag to control sending data
bool sendData = true; // Flag to control if data should be sent


byte aesKey[16] = {'h', 'e', 'l', 'l', 'o', 's', 'n', 'u', 'a', 'p', 'p', 'l', 'y', 'c', 'y', 'b'};
byte aes_iv[16] = {0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef};

AESLib aesLib;
char decrypted[64];

String aesDecrypt(String encryptedMessage) {
  byte encrypted[128];
  byte ciphertext[112];  // max length after removing IV
  byte decrypted_bytes[64]; // decrypted message bytes

  // Base64 decode
  int len = base64_decode((char *)encrypted, encryptedMessage.c_str(), encryptedMessage.length());

  // Extract IV from the first 16 bytes
  for (int i = 0; i < 16; i++) {
    aes_iv[i] = encrypted[i];
  }

  // Extract the ciphertext (after the first 16 bytes)
  for (int i = 16; i < len; i++) {
    ciphertext[i - 16] = encrypted[i];
  }

  // Decrypt message
  int decryptedLen = aesLib.decrypt(ciphertext, len - 16, decrypted_bytes, aesKey, 128, aes_iv);

  // Convert decrypted byte array to string
  String decryptedMessage = "";
  for (int i = 0; i < decryptedLen; i++) {
    decryptedMessage += (char)decrypted_bytes[i];
  }

  return decryptedMessage;
}

// Function to generate random temperature with noise
float generateNoisyTemperature() {
  // Mean temperature between 20 and 30 °C with noise +/- 5 °C
  float mean = 25.0;  
  float noise = random(-500, 500) / 100.0; // Noise range: -5.00 to +5.00
  return mean + noise;
}

// Function to generate random humidity with noise
float generateNoisyHumidity() {
  // Mean humidity between 40 and 60 % with noise +/- 20 %
  float mean = 50.0; 
  float noise = random(-200, 200) / 100.0; // Noise range: -20.0 to +20.0
  return mean + noise;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  // Attempt to connect to WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

// Function to fetch location and weather data when an anomaly is detected
void fetchLocationAndCompareWeather(float anomalyTemp, float anomalyHumd, bool isTemp) {
  if (WiFi.status() == WL_CONNECTED) {
    // Fetch location data using IP geolocation API
    HTTPClient http;
    const char* geoApiUrl = "http://ip-api.com/json/";
    http.begin(espClient, geoApiUrl);  // Initialize HTTP client with IP geolocation API

    int httpCode = http.GET();  // Send GET request
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Location Data:");
      Serial.println(payload);

      // Parse JSON to extract latitude and longitude
      StaticJsonDocument<512> doc;
      deserializeJson(doc, payload);
      float latitude = doc["lat"];
      float longitude = doc["lon"];
      Serial.print("Latitude: ");
      Serial.println(latitude);
      Serial.print("Longitude: ");
      Serial.println(longitude);

      // Use coordinates to get weather data
      fetchWeather(latitude, longitude, anomalyTemp, anomalyHumd, isTemp);
    } else {
      Serial.println("Error on HTTP request for location");
    }
    http.end();  // Close connection
  } else {
    Serial.println("Wi-Fi Disconnected");
  }
}

void fetchWeather(float latitude, float longitude, float anomalyTemp, float anomalyHumd, bool isTemp) {
  if (WiFi.status() == WL_CONNECTED) {
    char apiUrl[256];
    snprintf(apiUrl, sizeof(apiUrl), "http://api.openweathermap.org/data/2.5/weather?lat=%f&lon=%f&appid=%s&units=metric", latitude, longitude, "e24cbbcc9c0dff7b68d0174bd3b7b21f");

    HTTPClient http;
    http.begin(espClient, apiUrl);  // Initialize HTTP client with weather API URL

    int httpCode = http.GET();  // Send GET request
    if (httpCode > 0) {
      String payload = http.getString();
      Serial.println("Weather Data:");
      Serial.println(payload);

      // Parse JSON to extract temperature and humidity
      StaticJsonDocument<512> weatherDoc;
      deserializeJson(weatherDoc, payload);
      float temperature = weatherDoc["main"]["temp"];
      float humidity = weatherDoc["main"]["humidity"];
      Serial.print("Weather Temperature: ");
      Serial.print(temperature);
      Serial.println(" °C");
      Serial.print("Weather Humidity: ");
      Serial.print(humidity);
      Serial.println(" %");

      // Compare the fetched data with the anomaly data
      if (isTemp) {
        // Temperature anomaly
        float tempDiff = abs(temperature - anomalyTemp);
        Serial.print("Temperature difference: ");
        Serial.println(tempDiff);

        // Negate anomaly if difference is within 3°C
        if (tempDiff <= 3) {
          Serial.println("Anomaly temperature matches weather data, negating anomaly.");
          sendData = true;  // Allow data to be sent
          digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate action
          delay(500);  // LED stays on for half a second
          digitalWrite(LED_PIN, LOW);   // Turn off LED
        } else {
          Serial.println("Anomaly temperature remains valid.");
          digitalWrite(LED_PIN, HIGH);  // Keep LED on to indicate anomaly is valid
        }
      } else {
        // Humidity anomaly
        float humdDiff = abs(humidity - anomalyHumd);
        Serial.print("Humidity difference: ");
        Serial.println(humdDiff);

        // Negate anomaly if difference is within 10%
        if (humdDiff <= 10) {
          Serial.println("Anomaly humidity matches weather data, negating anomaly.");
          sendData = true;  // Allow data to be sent
          digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate action
          delay(500);  // LED stays on for half a second
          digitalWrite(LED_PIN, LOW);   // Turn off LED
        } else {
          Serial.println("Anomaly humidity remains valid.");
          digitalWrite(LED_PIN, HIGH);  // Keep LED on to indicate anomaly is valid
        }
      }
    } else {
      Serial.println("Error on HTTP request for weather data");
    }
    http.end();  // Close connection
  } else {
    Serial.println("Wi-Fi Disconnected");
  }
}

// MQTT callback function for handling incoming messages
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }

  // Print the full message to the serial monitor
  Serial.println(message);

  // If the message is from the "anamoly" topic
  if (String(topic) == topic_anomaly) {
    String value = "";
    String nodeData = "";
    
    //String message = aesDecrypt(message);
    Serial.print("Decrypted Message: ");
    Serial.println(message);
    // Check if the message starts with "[temp]" or "[humd]"
    if (message.startsWith("[temp]")) {
      // Extract the value for temperature (e.g., "[temp][29]" -> "29")
      int startIndex = message.indexOf("[", 6) + 1;  // After "[temp]" 
      int endIndex = message.indexOf("]", startIndex);

      if (startIndex != -1 && endIndex != -1) {
        value = message.substring(startIndex, endIndex);  // Extract temperature value
        nodeData = message.substring(endIndex + 1);  // Extract remaining part (e.g., "node1")
      }
      // Debug output
      Serial.print("Extracted temp value: ");
      Serial.println(value);
      Serial.print("Extracted node data: ");
      Serial.println(nodeData);

      // Further processing: Check if node1 exists in nodeData
      if (nodeData == "node1") {
        Serial.println("Anomaly detected! Fetching location and weather data for temperature.");
        sendData = false; // Stop sending data
        digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate anomaly detection

        // Fetch location and weather data for temperature
        float anomalyTemp = value.toFloat();  // Get the temperature value from anomaly message
        fetchLocationAndCompareWeather(anomalyTemp, 0, true);  // true indicates it's for temperature
      }

    } else if (message.startsWith("[humd]")) {
      // Extract the value for humidity (e.g., "[humd][45]" -> "45")
      int startIndex = message.indexOf("[", 6) + 1;  // After "[humd]"
      int endIndex = message.indexOf("]", startIndex);

      if (startIndex != -1 && endIndex != -1) {
        value = message.substring(startIndex, endIndex);  // Extract humidity value
        nodeData = message.substring(endIndex + 1);  // Extract remaining part (e.g., "node1")
      }
      // Debug output
      Serial.print("Extracted humd value: ");
      Serial.println(value);
      Serial.print("Extracted node data: ");
      Serial.println(nodeData);

      // Further processing: Check if node1 exists in nodeData
      if (nodeData == "node1") {
        Serial.println("Anomaly detected! Fetching location and weather data for humidity.");
        sendData = false; // Stop sending data
        digitalWrite(LED_PIN, HIGH);  // Turn on LED to indicate anomaly detection

        // Fetch location and weather data for humidity
        float anomalyHumd = value.toFloat();  // Get the humidity value from anomaly message
        fetchLocationAndCompareWeather(0, anomalyHumd, false);  // false indicates it's for humidity
      }
    }
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      client.subscribe(topic_anomaly);  // Subscribe to anomaly topic
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup() {
  // Initialize Serial Monitor
  Serial.begin(9600);

  // Set LED pin as output
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW); // Ensure LED is off initially

  // Initialize Wi-Fi
  setup_wifi();

  // Initialize MQTT client and set callback
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  // Initialize DHT sensor
  dht.begin();
  Serial.println("DHT11 sensor initialized.");
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  // If sendData is false, skip reading and publishing sensor data
  if (!sendData) {
    Serial.println("Data sending is stopped due to anomaly detection.");
    return;  // Skip the rest of the loop
  }

  // Read temperature and humidity from DHT sensor
  float temperature = dht.readTemperature();  // Reading in Celsius
  float humidity = dht.readHumidity();

  // If temperature is NaN, generate a noisy random temperature
  if (isnan(temperature)) {
    Serial.println("Temperature is NaN, generating random noisy temperature.");
    temperature = generateNoisyTemperature();
  }

  // If humidity is NaN, generate a noisy random humidity
  if (isnan(humidity)) {
    Serial.println("Humidity is NaN, generating random noisy humidity.");
    humidity = generateNoisyHumidity();
  }

  // Debugging: Print raw data values to the serial monitor
  Serial.print("Temperature: ");
  Serial.print(temperature);
  Serial.print("°C  ");
  Serial.print("Humidity: ");
  Serial.print(humidity);
  Serial.println("%");

  // Create messages to publish
  String tempMessage = "Temperature: " + String(temperature) + "°C";
  String humidityMessage = "Humidity: " + String(humidity) + "%";

  // Publish temperature
  client.publish(topic1, tempMessage.c_str());
  Serial.print("Published on ");
  Serial.print(topic1);
  Serial.print(": ");
  Serial.println(tempMessage);

  // Publish humidity
  client.publish(topic2, humidityMessage.c_str());
  Serial.print("Published on ");
  Serial.print(topic2);
  Serial.print(": ");
  Serial.println(humidityMessage);

  // Wait for 5 seconds before publishing again
  delay(5000);
}