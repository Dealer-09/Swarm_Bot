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

## Repository Structure

Below is a breakdown of the codebase and what each directory contains:

```text
Swarm_Bot/
|-- plan.md                                # The master 1000-line project specification and phase plan.
|-- README.md                              # This file (Hardware info, pin mappings, project status).
|-- .gitignore                             # Git ignore rules for Python, PlatformIO, and temp files.
|-- yolo11n.pt                             # The original PyTorch YOLO11n model weights.
|
|-- phase0_setup/                          # Scripts for initial robot provisioning
|   |-- install_deps.sh                    # Bash script to create Python venvs and install ncnn/opencv on the Pis.
|   |-- verify_install.sh                  # Verifies Python packages are installed correctly.
|   |-- check_hardware.sh                  # Verifies USB devices (webcam, Arduino, XBee) are visible to Linux.
|   |-- identify_ports.sh                  # Determines which /dev/ttyUSB* is the Arduino and which is the XBee.
|   |-- export_yolo_ncnn.py                # Windows script used to convert the PyTorch model to NCNN format.
|   |-- deploy_phase0.ps1                  # PowerShell script to SCP files to the robots.
|   |-- yolo11n_ncnn_model/                # The exported YOLO NCNN model directory
|       |-- model.ncnn.bin                 # NCNN binary weights (5.1MB)
|       |-- model.ncnn.param               # NCNN network architecture graph
|       |-- metadata.yaml                  # Model metadata (classes, image size)
|
|-- phase1_arduino/                        # Arduino firmware for motor control
|   |-- platformio.ini                     # PlatformIO build configuration for Arduino Uno.
|   |-- sketch_pi_motor_bridge/
|       |-- sketch_pi_motor_bridge.ino     # The final, verified firmware that listens to the Pi over serial and drives motors.
|   |-- sketch_motor_diag3/                # Diagnostic firmware used to discover the correct A0-A3 pin mappings.
|
|-- phase3_vision/                         # Live video streaming and AI testing
|   |-- yolo_stream.py                     # Python script running on the Pi that streams the webcam + YOLO boxes to a web browser.
|
|-- shared/                                # Code shared between both Robot A and Robot B
|   |-- yolo_ncnn.py                       # Custom Python wrapper for NCNN inference (handles image resizing, decoding, and NMS).
|
|-- robot_a/                               # Future directory: Main tracking loop for Robot A (Leader)
|-- robot_b/                               # Future directory: Main response loop for Robot B (Follower)
```
