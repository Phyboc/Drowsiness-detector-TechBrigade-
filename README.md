# Drowsiness Detector by TechBrigade

## 🚗 Project Overview
This project aims to detect driver drowsiness using **ESP32-CAM** and **ESP32-DevKit**.  
It monitors **eye blinking** and **head tilting** using an Edge Impulse ML model.  
If the driver’s eyes remain closed for more than 2 seconds or head tilts beyond 45°,  
a buzzer is triggered to alert them.

## ⚙️ Features
- ESP32-CAM with Edge Impulse ML model for eye/head detection.
- IoT-based remote alerts using **Blynk**.
- Simulation & prototyping with **Wokwi**.
- MERN-based web app for monitoring.

## 🛠️ Tech Stack
- **Hardware:** ESP32-CAM, ESP32-DevKit, Buzzer, Battery
- **Software:** Arduino IDE, Edge Impulse, Wokwi, Blynk
- **Web App:** MERN stack
- **Cloud:** Blynk IoT platform

## 📂 Repository Structure
See the folder structure in this repo for:
- `hardware/` → circuits & components
- `firmware/` → ESP32 code
- `ml-model/` → Edge Impulse files
- `app/` → web/mobile app
- `docs/` → project documentation

## 🔗 Important Links
- [Dataset (Edge Impulse)](https://studio.edgeimpulse.com/public/168098/latest)
- [Wokwi Simulation](https://wokwi.com/projects/437271120295952385)

## 👨‍💻 Team TechBrigade
- Sivasubramani K J (Lead)  
- Govind Nair  
- Samanyu Nair  
- Shreyas N  

## 📜 License
This project is licensed under the MIT License.
