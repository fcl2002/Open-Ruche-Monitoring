# Open Ruche 🐝 — BEE SMART

**Système de Monitorage Autonome de Ruches en Temps-réel**  
_Protéger les abeilles, préserver demain._

An intelligent beehive monitoring solution combining embedded AI, LoRaWAN communication, and solar power to help beekeepers protect their colonies and optimize hive management.

---

## Context & Motivation

Bee colonies face an accelerating global decline driven by the *Varroa destructor* mite, the Asian hornet (*Vespa velutina*), pesticides, and climate change. Continuous 24/7 manual monitoring is impossible for beekeepers — Open Ruche fills that gap with autonomous, real-time surveillance and automatic threat detection.

---

## Team

| Role | Name |
|------|------|
| **Project Manager, System Integration & Firmware Lead** | [Fernando COSTA LASMAR](https://www.linkedin.com/in/fernando-costa-lasmar/) |
| **AI/ML Engineer & Model Development** | [Flávio ROSIM DE SOUSA](https://www.linkedin.com/in/flávio-rosim-de-sousa/) |
| **Hardware Design & PCB Engineer** | [Matheus SISTON GALDINO](https://www.linkedin.com/in/matheussistongaldino/) |
| **Power Management & Solar Systems** | [Karim SAWAYA](https://www.linkedin.com/in/karim-sawaya-67ab87292/) |
| **HMI & Documentation** | [Yan DING](mailto:dingyan02040608@gmail.com) |

**Academic context:** Polytech Sorbonne — EI4 FISE | Projet Systèmes Embarqués IoT 2025-2026 | Groupe 4 – Ruche 9

---

## Solution Overview

Open Ruche is built around four integrated layers:

**Power Layer**
- Solar panel + battery with charge management module
- Switchable voltage regulator for load optimization
- Deep Sleep mode bringing average draw down to ~0.822 mA

**Sensor Layer (custom PCB)**
- Temperature: 2× DS18B20 probes (internal) + 1× DHT22 (external T°/humidity)
- Weight: HX711 load cell amplifier
- Light: LUX sensor
- Motion/safety: MMA8452 accelerometer, GPS module, buzzer alarm
- Air quality: gas/particle sensor (SEN0190)

**AI Processing Layer**
- Dual-core setup: Arduino Nano 3.3 (audio) + XIAO ESP32S3 (vision)
- TensorFlow Lite models running fully on-device
- **Audio classification model** — 86.45% accuracy on test data
  - Class 1: Normal activity
  - Class 2: Swarming
  - Class 3: Queenless colony
- **Hornet detection model (computer vision)** — 79.7% accuracy, F1-score 90%
  - Code 0: No hornet detected
  - Code 1: Hornet detected

**Communication & Cloud Layer**
- LoRaWAN module → The Things Network (TTN)
- Configurable uplink frequency via TTN downlink
- Cloud dashboards: BEEP Monitor + Datacake
- Real-time alerts and remote parameter configuration

---

## Software Architecture

The firmware implements a 4-state machine to minimise power consumption:

```
BOOT → INIT → LECTURE (sensors + AI) → DEEP SLEEP
              ↑___________________________________|
              Wake after configurable interval
```

- **INIT → LECTURE** triggered when LUX > 100 AND internal temp > 20 °C  
- **LECTURE → DEEP SLEEP** when LUX < 100 OR internal temp < 15 °C  
- LoRa downlink allows remote configuration of the sleep interval  
- Logging system for debugging and field maintenance

---

## Power Consumption

| Phase | Average Current |
|-------|----------------|
| Active period (AI + sensors + LoRa) | ~85.8 mA |
| Deep Sleep | ~0.822 mA |

Charge breakdown per cycle (54 s total):
- AI (10 s): 0.414 mAh — 38%
- Sensors (15 s): 0.283 mAh — 26%
- LoRa (9 s): 0.117 mAh — 11%
- Deep Sleep (20 min): 0.274 mAh — 25%

**Estimated autonomy (1000 mAh battery):** ~13 days without solar charging.

---

## Project Status

| Objective | Status |
|-----------|--------|
| Sensor accuracy & LoRa transmission | ✅ Done |
| Real-time image & audio AI detection | ✅ Done |
| Alarm creation & notifications | ✅ Done |
| Remote parameter & alarm configuration | ✅ Done |
| System energy autonomy | ✅ Done |
| PCB schematic & assembly | ✅ Done |
| Enclosure fabrication & prototype wiring | ✅ Done |
| Data display on cloud platforms | ✅ Done |
| Accelerometer for fall/theft detection | ❌ Not implemented |

---

## Getting Started

### Prerequisites

- Arduino IDE or PlatformIO
- KiCad (for hardware design)
- The Things Network account + LoRaWAN gateway access

### Quick Start

```bash
# Clone the repository
git clone https://github.com/[your-username]/OpenRuche.git
cd OpenRuche

# Project structure
# hardware/   — KiCad schematics & PCB files
# software/   — Firmware (Arduino / PlatformIO)
# ai/         — TensorFlow Lite model training scripts
# docs/       — Documentation & guides
```

For detailed development guidelines, see [CONTRIBUTING.md](CONTRIBUTING.md).

---

## License

This project is open-source. See [LICENSE](LICENSE) for details.