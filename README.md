# LoRa-Based AI-Assisted Emergency Communication System

## 1. Project Overview

This project implements a **resilient emergency communication system** that operates **without cellular networks or internet dependency**. It uses **LoRa (SX1278) long-range radio modules** integrated with **ESP32 nodes** and a **Raspberry Pi command center** to form a decentralized, low-power, and fault-tolerant communication network.

Each ESP32 node hosts its own **Wi-Fi Access Point (AP)** and provides a **web-based user interface** for monitoring node status and sending messages. The Raspberry Pi acts as a **central command and monitoring station**, receiving LoRa packets and optionally intercepting them for logging and analysis.

The system is designed for **disaster response scenarios**, where conventional communication infrastructure is unavailable or unreliable.

---

## 2. System Architecture

### 2.1 High-Level Architecture

* **ESP32 + SX1278 nodes**

  * Act as field communication nodes
  * Transmit and receive LoRa messages
  * Host a local web interface over Wi-Fi AP
* **LoRa Mesh Communication**

  * Long-range, low-power radio communication at 433 MHz
  * Infrastructure-independent
* **Raspberry Pi + SX1278**

  * Acts as a command relay and monitoring station
  * Displays received packets via a web dashboard
  * Supports interceptor mode for packet logging

---

## 3. Hardware Requirements

### 3.1 ESP32 Node Components

* ESP32 Development Board
* SX1278 LoRa Module (433 MHz)
* 433 MHz Antenna
* Jumper wires
* Power supply (USB / battery)

### 3.2 Raspberry Pi Components

* Raspberry Pi (Zero 2 W / 3 / 4)
* SX1278 LoRa Module (433 MHz)
* 433 MHz Antenna
* Jumper wires
* Power supply

---

## 4. Circuit Connections

### 4.1 ESP32 to SX1278 Connections

| ESP32 Pin | SX1278 Pin |
| --------- | ---------- |
| GND       | GND        |
| 3.3V      | VCC        |
| D5        | NSS        |
| D23       | MOSI       |
| D19       | MISO       |
| D18       | SCK        |
| D14       | RST        |
| D2        | DIO0       |

Important notes:

* Use **only 3.3V**, never 5V
* Ensure a proper 433 MHz antenna is connected

---

### 4.2 Raspberry Pi to SX1278 Connections

| Raspberry Pi Pin | SX1278 Pin   |
| ---------------- | ------------ |
| 3.3V             | 3.3V         |
| Ground           | Ground       |
| GPIO 10          | MOSI         |
| GPIO 9           | MISO         |
| GPIO 11          | SCK          |
| GPIO 8           | NSS / Enable |
| GPIO 4           | DIO0         |
| GPIO 17          | DIO1         |
| GPIO 18          | DIO2         |
| GPIO 27          | DIO3         |
| GPIO 22          | RST          |

---

## 5. Software Architecture

### 5.1 ESP32 Node Software

Each ESP32 node runs:

* LoRa transmitter and receiver
* Wi-Fi Access Point (AP mode)
* HTTP web server (port 80)
* Local dashboard with login authentication
* Message sending interface
* Real-time status reporting (RSSI, packet count)

The ESP32 uses **LittleFS** to store web files.

### 5.2 Raspberry Pi Software

The Raspberry Pi runs:

* Python-based LoRa receiver
* Flask web server (port 80)
* Command center dashboard
* Interceptor mode (toggleable)
* Live packet logging

---

## 6. Tech Stack Used

### Hardware

* ESP32
* Raspberry Pi
* SX1278 LoRa Module (433 MHz)

### Firmware / Embedded

* Arduino Framework
* LoRa (Sandeep Mistry library)
* ESPAsyncWebServer
* AsyncTCP
* LittleFS

### Backend / Server

* Python 3
* Flask

### Frontend

* HTML5
* CSS3 (dark tactical theme)
* JavaScript (Fetch API)

---

## 7. ESP32 Firmware

### 7.1 LoRa Configuration (Fixed)

The following parameters are fixed and must match across all devices:

* Frequency: **433 MHz**
* Sync Word: **0xA5**
* Spreading Factor: **7**
* Bandwidth: **125 kHz**
* Coding Rate: **4/5**

These values ensure compatibility between ESP32 and Raspberry Pi.

---

### 7.2 ESP32 Main Code

The ESP32 firmware:

* Initializes LoRa
* Creates a Wi-Fi hotspot
* Mounts LittleFS
* Serves login and dashboard pages
* Sends and receives LoRa messages
* Exposes REST endpoints for status and messaging

(Refer to the provided `.ino` file used in the project.)

---

## 8. ESP32 Web Interface

### Features

* Hard-coded login authentication
* Real-time node status
* Packet count and RSSI display
* Message sending via LoRa
* Auto-refresh using JavaScript

### File Structure (LittleFS)

```
/data
 ├── login.html
 ├── dashboard.html
 ├── style.css
 └── app.js
```

These files are uploaded using the **LittleFS filesystem uploader**.

---

## 9. Raspberry Pi Software

### 9.1 Dependencies Installation

```bash
sudo apt update
sudo apt install python3-pip
pip3 install flask spidev RPi.GPIO
```

SPI must be enabled using `raspi-config`.

---

### 9.2 Raspberry Pi LoRa Receiver

* Continuously listens for LoRa packets
* Matches ESP32 LoRa parameters
* Extracts payload and RSSI
* Forwards packets to the command center UI

---

### 9.3 Command Center Web Interface

* Runs on port 80
* Displays live packet logs
* Interceptor mode:

  * OFF: passive monitoring
  * ON: intercepted packet logging
* Dark tactical dashboard design

---

## 10. Execution Workflow

### 10.1 ESP32

1. Upload LittleFS web files
2. Upload ESP32 firmware
3. Power ESP32
4. Connect to Wi-Fi:

   * SSID: `EMERGENCY-NODE-01`
   * Password: `admin123`
5. Open browser:

   ```
   http://192.168.4.1
   ```
6. Login:

   * Username: `admin`
   * Password: `lora123`

---

### 10.2 Raspberry Pi

1. Power Raspberry Pi
2. Run LoRa receiver and Flask server:

   ```bash
   sudo python3 app.py
   ```
3. Access dashboard:

   ```
   http://<raspberry_pi_ip>
   ```

---

## 11. System Working

1. ESP32 sends LoRa packets
2. Raspberry Pi receives packets via SX1278
3. Packets are displayed on the command center UI
4. ESP32 dashboard shows live node status
5. Interceptor mode allows selective packet handling

The system continues to operate **without internet or cellular connectivity**.

---

## 12. Applications

* Disaster response communication
* Emergency coordination
* Remote area messaging
* Military or tactical field communication
* Research on resilient wireless networks

---

## 13. Limitations

* Limited LoRa data rate
* No end-to-end encryption (can be added)
* Single-channel LoRa communication
* Web UI authentication is basic (hard-coded)

---

## 14. Future Enhancements

* AI-based adaptive routing
* Message encryption (AES)
* Multi-node mesh routing
* WebSocket-based live streaming
* Role-based authentication
* Battery-aware routing
* UAV-assisted relay nodes

---
