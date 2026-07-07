# 🌊 संकेतEdge

> **An Edge AI-powered Multi-Hazard Disaster Monitoring System**
> Built for **HACKSAGON 2025** (ABV-IIITM Gwalior × IEEE MP Section)

## 📖 Overview

Natural disasters such as floods and droughts often require rapid local decision-making, even in areas with unreliable internet connectivity. **संकेतEdge** is a low-cost, distributed disaster monitoring system that brings intelligence directly to the edge.

Instead of sending raw sensor data to the cloud for analysis, each sensing node processes data locally, classifies risk, and communicates only essential information over LoRa. A receiver node fuses these hazard predictions, triggers local alerts without internet access, and updates a cloud dashboard for monitoring and explainability.

This offline-first design enables faster response, lower bandwidth usage, and improved resilience during emergencies.

---

## 🚀 Key Features

* 🌊 Flood detection using an ESP32 and waterproof ultrasonic sensor
* 🌱 Drought monitoring using soil moisture, temperature, and humidity sensors
* 🧠 TinyML-based flood classification running directly on the ESP32
* ⚡ Edge computing for drought risk assessment
* 📡 Long-range LoRa communication between distributed nodes
* 🔀 Receiver-based multi-hazard edge fusion
* 🚨 Offline local alerts using buzzer/LED without internet dependency
* ☁️ Cloud dashboard for visualization and explainable monitoring

---

# 🏗️ System Architecture

```
                Flood Detection Node
        ESP32 + JSN-SR04T + TinyML + LoRa
                    │
                    │
                    ▼
             Receiver / Edge Fusion Hub
      ESP32 + LoRa + Wi-Fi + Local Alerts
                    ▲
                    │
                    │
      ESP32 + Soil Sensor + DHT11 + LoRa
              Drought Detection Node
```

---

## ⚙️ How It Works

### 🌊 Node 1 : Flood Detection

* Measures water level using the JSN-SR04T ultrasonic sensor.
* Converts distance into water level percentage.
* Runs a TinyML model trained using Edge Impulse.
* Classifies flood conditions into:

  * NORMAL
  * WARNING
  * DANGER
* Detects rainfall intensity by monitoring changes in water level.
* Sends flood status and rainfall information through LoRa.

---

### 🌱 Node 2 : Drought Detection

* Measures soil moisture.
* Reads temperature and humidity using the DHT11 sensor.
* Performs on-device edge analysis.
* Estimates drought risk as:

  * LOW
  * MEDIUM
  * HIGH
* Sends processed information through LoRa.

---

### 🧠 Receiver : Edge Fusion Hub

The receiver acts as the central intelligence of the system.

It:

* Receives LoRa packets from both sensing nodes.
* Combines flood and drought information.
* Performs multi-hazard risk analysis.
* Triggers buzzer/LED alerts locally.
* Sends processed data to the cloud dashboard using Wi-Fi.

Because alerts are generated directly on the ESP32, the system continues functioning even when internet connectivity is unavailable.

---

# 🛠️ Hardware Used

* ESP32 Development Boards ×3
* SX1278 LoRa Modules ×3
* JSN-SR04T Waterproof Ultrasonic Sensor
* Soil Moisture Sensor
* DHT11 Temperature & Humidity Sensor
* Breadboards and jumper wires

---

# 💻 Software & Tools

* Arduino IDE
* Edge Impulse
* ESP32 Arduino Framework
* LoRa Library
* ArduinoJson
* Blynk IoT
* GitHub

---

# 📂 Repository Structure

```
sanket_edge/
│
├── node1_flood_detection/
├── node2_drought_detection/
├── receiver_edge_fusion_hub/
├── docs/
│   └── SanketEdge_HACKSAGON2025_Presentation.pdf
└── README.md
```

---

# 💡 What Makes संकेतEdge Different?

Unlike many disaster monitoring systems that rely heavily on cloud processing, SanketEdge performs decision-making directly on embedded devices.

The project combines:

* Distributed edge intelligence
* TinyML on resource-constrained hardware
* Dual-hazard monitoring in one system
* Long-range LoRa communication
* Offline emergency alerts
* Cloud visualization for explainability rather than dependency

---

# 👥 Team

**Team Name:** Naive Birds

### Kanak Tembhurne

* Edge Computing
* TinyML Model Development (Edge Impulse)


### Priyanshu Patil

* Hardware Integration
* Frontend Dashboard

  
---

# 🏆 Hackathon

**HACKSAGON 2025**

Organized by **ABV-IIITM Gwalior** in collaboration with the **IEEE MP Section**.

---

# 📄 Project Presentation

The project presentation is available in:

```
docs/SanketEdge_HACKSAGON2025_Presentation.pdf
```

---

# 🌱 Future Improvements

* GPS-enabled emergency localization
* Additional hazard monitoring (wildfire, landslides)
* Solar-powered remote deployments
* Mobile application integration
* Adaptive AI models for seasonal changes

---

 ⭐ If you found this project interesting, feel free to star the repository and explore the code. Contributions, feedback, and suggestions are always welcome.
