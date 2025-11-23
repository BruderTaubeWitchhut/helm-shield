# **HelmShield – Smart Helmet Accident Detection & Emergency Alert System**

HelmShield is a compact, real-time accident-detection system designed for two-wheeler riders.  
A lightweight helmet-mounted sensing node detects crashes instantly and wirelessly alerts a gateway, which then sends the rider’s GPS location to their emergency contact via SMS and an automated call.

---

## 🚨 Overview

HelmShield consists of two nodes:

### **1. Helmet Sensor Node (XIAO ESP32)**
- MPU6050 (3-axis accelerometer + gyro)  
- Vibration sensor   
- Sends crash alerts using **ESP-NOW**  
- Runs off a small helmet-mounted battery

### **2. BharatPi Gateway Node (ESP32)**
- Connected to **GPS module** + **SIM800L** GSM modem  
- Receives crash packets from helmet  
- Sends SMS + places an automated call to the rider’s emergency contact  
- Includes fallback and retry logic for reliability

---

## 🔄 System Flow

1. **Impact Detected** — The XIAO node analyzes MPU6050 changes (Δa), gyro spikes, and vibration sensor triggers.  
2. **Pre-Alert Countdown** — Rider gets 10–20 seconds to cancel false alerts.  
3. **Alert Transmission** — A compact binary packet is sent via ESP-NOW to the BharatPi Node.  
4. **Emergency Notification** — The gateway retrieves GPS coordinates and:  
   - Sends SMS with Google Maps link  
   - Initiates a voice call  
   - Logs all events for debugging

---

## 🎯 Accident Detection Logic

- Sudden acceleration spike  
- Gyro instability within a short window  
- Vibration sensor confirmation  
- Cancel option  
- Cooldown to prevent repeated alerts

---

## 📡 Communication

- **Local:** XIAO → Gateway using ESP-NOW  
- **External:** SIM800L GSM module for SMS + call  
- **App:**  
  - Setup emergency contacts  
  - Manual panic button  
  - See device status (battery, last heartbeat, last alert)

---

## 🔧 Hardware List

- XIAO ESP32 (or similar)  
- MPU6050  
- Vibration sensor  
- Push button + buzzer  
- ESP32 Dev Board  
- GPS Module (NEO-6M or similar)  
- SIM800L GSM Module  
- Power supply capable of handling **2A GSM peaks**

---

## 💾 Packet Structure

Packet includes:
- Device ID  
- Timestamp  
- IMU snapshot  
- Vibration flag  
- Event type  
- Sequence number  
- CRC16 checksum  

Gateway returns an ACK. Retries used for reliability.

---

## 🔋 Power & Reliability Notes

- SIM800L requires low-ESR capacitor  
- Ensure stable power routing to handle GSM transmission peaks  
- Store last-known GPS fix if fresh fix unavailable  
- Log events to EEPROM/SPIFFS for later debugging  

---

## 📱 User Experience

- Low-latency automatic crash alerts  
- Buzzer warning + cancel button  
- SMS includes a Google Maps link  
- Optional app for configuration and manual panic button

---

## 👥 Team

**Dyutikar G V** — Software  
**Hridan Saikia** — Hardware  
**Tanmay S U** — App Development  

---

## 📁 Project Structure

helm-shield/
│── HelmShield_Synthax-Sender.ino
│── HelmShield_Synthax-Receiver.ino
│── README.md

---

## 🚀 Future Enhancements

- Add support for ADXL345 / GY-45 accelerometers  
- Bluetooth provisioning in the app  
- Battery and health diagnostics  
- Machine-learning-based crash prediction  

---

## 🤖 AI Usage

ChatGPT was used **only for debugging errors in the code**, documentation assistance, and refining logic descriptions.  
All hardware design, algorithms, and system-level decisions were made by the development team.
