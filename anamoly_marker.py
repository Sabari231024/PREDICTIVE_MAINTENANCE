import paho.mqtt.client as mqtt 
import time 
import random 

broker = "192.168.247.122"  # Your broker's IP 
port = 1883               # Default MQTT port 
topic1 = "sensor/anamoly1" 

client = mqtt.Client() 
client.connect(broker, port) 
from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.primitives import padding
from cryptography.hazmat.backends import default_backend
from base64 import b64encode, b64decode

def encrypt_message(message, key):
    # Key must be 16 bytes (AES-128)
    key = key.encode('utf-8')[:16]

    # Initialization Vector (same as in ESP8266 code)
    iv = bytes([0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef, 0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef])

    # Create a Cipher object
    cipher = Cipher(algorithms.AES(key), modes.CBC(iv), backend=default_backend())
    encryptor = cipher.encryptor()

    # Add padding to the message (PKCS7)
    padder = padding.PKCS7(128).padder()
    padded_message = padder.update(message.encode('utf-8')) + padder.finalize()

    # Encrypt the padded message
    ciphertext = encryptor.update(padded_message) + encryptor.finalize()

    # Concatenate IV and ciphertext, then Base64 encode
    encrypted_data = iv + ciphertext
    return b64encode(encrypted_data).decode('utf-8')

try: 
    while True: 
        #user_input = input("Enter 'tempdata': ")
        #message1 = f"[temp][{user_input}]node1" 
        message1="[temp][51.95]node1"
        #message1 = encrypt_message(message1, "hellosnuapplycyb")
        client.publish(topic1, message1) 
        print(f"Published on {topic1}: {message1}") 
        time.sleep(5) 
except KeyboardInterrupt: 
    print("Publishing stopped by user.") 
finally: 
    client.disconnect()