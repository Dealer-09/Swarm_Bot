# Swarm Ground Proof-of-Concept (POC)

This repository contains the software stack for a two-robot ground swarm proof-of-concept. It serves as the foundation for autonomous coordination, person-tracking, and XBee-based communication.

## Hardware Stack (Per Robot)
*   **Base:** Waveshare AlphaBot2-Base (contains TB6612FNG motor driver & 2x 14500 Li-ion batteries)
*   **Adapter Shield:** AlphaBot2-Ar (Arduino UNO compatible adapter)
*   **Microcontroller:** Arduino UNO R3 (Mounted on the AlphaBot2-Ar shield)
*   **Companion Computer:** Raspberry Pi 4B (4GB RAM)
*   **Vision:** USB Webcam (Jieli Technology)
*   **RF Communication:** XBee-PRO S2C (Zigbee TH PRO, 9600 baud, Transparent mode)

## Critical Hardware Discoveries & Pin Mappings
The official documentation for the AlphaBot2-Ar is misleading regarding motor pins. The true pin mapping is defined by the yellow "Control JMP" jumper matrix on the AlphaBot2-Ar shield itself. 

**The motor direction pins are wired to the Arduino's Analog pins:**
*   **Left Motor (A):**
    *   PWM: `D6`
    *   AIN1: `A1`
    *   AIN2: `A0`
*   **Right Motor (B):**
    *   PWM: `D5`
    *   BIN1: `A2`
    *   BIN2: `A3`

*(Note: The Arduino firmware handles these analog pins as digital outputs using `pinMode(A0, OUTPUT)`).*

## Power & Thermal Considerations
Running computer vision models (YOLO) forces the Raspberry Pi 4 CPU to 100% utilization. Because the Pi is simultaneously powering a webcam, an Arduino, and an XBee via USB, this creates massive instantaneous current spikes.

*   **Symptoms:** If the wall adapter or battery pack cannot supply 5V @ 3A (15W) instantly, the Pi will suffer a brown-out (Wi-Fi drop, crash, or hard reboot).
*   **Solution:** The YOLO NCNN inference is throttled via `num_threads=1` or `num_threads=2`. This limits CPU usage, reducing the frame rate to ~2-3 FPS, but ensures absolute power stability without requiring external active cooling or industrial power supplies.

## Software Architecture

### 1. Arduino Motor Bridge (`phase1_arduino/`)
*   Compiled using PlatformIO (`pio run`).
*   Listens on `/dev/ttyUSB1` at 115200 baud for ASCII commands from the Pi.
*   **Protocol:** `M,<leftSpeed>,<leftDir>,<rightSpeed>,<rightDir>` (e.g., `M,150,F,150,B`).
*   Includes a 2-second watchdog timer to halt motors if the Pi crashes or disconnects.

### 2. YOLO NCNN Vision Engine (`shared/yolo_ncnn.py`)
*   Uses a highly optimized, custom Python wrapper around the `ncnn` library.
*   No heavy dependencies (No PyTorch, no Ultralytics).
*   Runs a YOLO11n FP16 exported model directly on the Pi's ARM CPU.

### 3. Live Web Stream (`phase3_vision/`)
*   Hosts an MJPEG stream on port `5000` (e.g., `http://10.179.79.74:5000`).
*   Captures webcam frames, runs YOLO inference, draws bounding boxes, and serves them over the local Wi-Fi network for debugging.

## Network & Access
| Robot | Hostname | Current IP (DHCP) | Role |
|---|---|---|---|
| **Robot A** | `pi.local` | `10.179.79.74` | Tracker (Camera + YOLO + XBee TX) |
| **Robot B** | `pi2.local` | `10.179.79.184` | Reactor (XBee RX + Motor response) |

## Phase Map & Progress
*   [x] **Phase 0:** Hardware check & Pi user-space Python dependencies (`~/swarm_venv`).
*   [x] **Phase 1:** Arduino motor bridge firmware (PlatformIO).
*   [x] **Phase 2:** Pi -> Arduino serial motor control verified.
*   [x] **Phase 3:** YOLO11n live camera stream deployed and tested.
*   [ ] **Phase 4:** Tracker Setup (Assign IDs to detected persons).
*   [ ] **Phase 5:** Navigation Loop (PID control to keep person in frame center).
*   [ ] **Phase 6:** XBee RF Coordination (Broadcasting target data between swarm members).
*   [ ] **Phase 7:** Swarm Reaction (Robot B reacts to Robot A's vision data).
