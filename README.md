# Open Ruche 🐝

**IoT Beehive Monitoring System with Embedded AI**

An intelligent beehive monitoring solution combining embedded artificial intelligence, LoRaWAN communication, and solar power to help beekeepers protect their colonies and optimize hive management.

---

## Project Overview

Open Ruche is an embedded IoT system designed to monitor beehive health in real-time using advanced sensors and on-device AI. The system detects threats like the Asian hornet (_Vespa velutina_), tracks colony health metrics, and transmits data via LoRaWAN to cloud platforms for analysis and alerts.

### Key Features

- **Embedded AI**: On-device computer vision and audio analysis for hornet detection
- **LoRaWAN Connectivity**: Long-range, low-power wireless communication
- **Solar Powered**: Autonomous energy management for remote deployment
- **Real-time Monitoring**: Temperature, humidity, weight, sound, and visual analysis
- **Cloud Integration**: Compatible with BEEP Monitor and Datacake platforms
- **Low Power Design**: Optimized for battery life and solar harvesting

---

## Team Members

| Role                                                    | Name                                                                            |
| ------------------------------------------------------- | ------------------------------------------------------------------------------- |
| **Project Manager, System Integration & Firmware Lead** | **[Fernando COSTA LASMAR](www.linkedin.com/in/fernando-costa-lasmar/)**         |
| **AI/ML Engineer & Model Development**                  | **[Flávio ROSIM DE SOUSA](https://www.linkedin.com/in/flávio-rosim-de-sousa/)** |
| **Hardware Design & PCB Engineer**                      | **[Matheus SISTON GALDINO](https://www.linkedin.com/in/matheussistongaldino/)** |
| **Power Management & Solar Systems**                    | **[Karim SAWAYA](https://www.linkedin.com/in/karim-sawaya-67ab87292/)**         |
| **IHM & Documentation**                                 | **[Yan DING](mailto:dingyan02040608@gmail.com)**                                |

**Academic Context:**  
Polytech Sorbonne - EI4 FISE  
Projet Systèmes Embarqués IoT 2025-2026

---

## Getting Started

### Prerequisites

- Arduino IDE or PlatformIO
- Python 3.8+ (for AI training)
- KiCad (for hardware design)
- LoRaWAN gateway access (The Things Network account)

### Quick Start

```bash
# Clone the repository
git clone https://github.com/[your-username]/OpenRuche.git
cd OpenRuche

# Check the project structure
ls

# Hardware files: ./hardware/
# Software code: ./software/
# Documentation: ./docs/
```

For detailed development guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

### System Components:

**Power Layer:**

- Solar panels for autonomous energy
- Battery management with status monitoring
- Load switch for power optimization

**Sensor Layer (PCB):**

- Environmental: Temperature, humidity, light
- Mechanical: Weight (HX711), accelerometer
- Detection: Gas/particle sensors for air quality
- Safety: GPS tracking, alarm system
- Interface: Status LED, power button

**Processing Layer:**

- Dual AI cores (Arduino + XIAO ESP32S3)
- Embedded TensorFlow Lite models
- Vision and audio analysis for hornet detection

**Communication Layer:**

- LoRaWAN module for long-range transmission
- The Things Network gateway integration

**Cloud & Display Layer:**

- Web dashboard
- BEEP Monitor integration
- Real-time alerts and analytics
