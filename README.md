# Swarm Ground POC

Two-robot ground proof-of-concept for an autonomous swarm drone project.

## Hardware
- 2× AlphaBot2-Ar (ATmega328P + TB6612FNG motors)
- 2× Raspberry Pi 4B 4GB
- 2× USB Webcams
- 2× XBee-PRO S2C (Zigbee TH PRO firmware, USB carrier boards)
  - Radio A: COM5 / MAC `0013A200 41F52FD2`
  - Radio B: COM6 / MAC `0013A200 41F52F20`
- 2× USB Type-A to Type-B cables (Pi → Arduino)

## Network
| Robot | SSH | Role |
|---|---|---|
| Robot A | `ssh pi@pi.local` | Tracker (Camera + YOLO + XBee TX) |
| Robot B | `ssh pi2@pi2.local` | Reactor (XBee RX + Motor response) |

## Phase Map
| Phase | Directory | Status |
|---|---|---|
| 0 — Hardware check + deps install | `phase0_setup/` | 🔄 In progress |
| 1 — Arduino motor bridge sketch | `phase1_arduino/` | ⏳ Pending |
| 2 — Pi → Arduino serial test | `robot_a/motor_interface/` | ⏳ Pending |
| 3 — USB webcam test | `robot_a/camera/` | ⏳ Pending |
| 4 — YOLO11n detection | `robot_a/detector/` | ⏳ Pending |
| 5 — ByteTrack tracking | `robot_a/tracker/` | ⏳ Pending |
| 6 — Robot A person-following | `robot_a/` | ⏳ Pending |
| 7 — XBee serial test | ✅ Done (XCTU) | ✅ Complete |
| 8 — Pi↔Pi XBee API mode | `robot_a/xbee_transport/` | ⏳ Pending |
| 9 — TARGET_UPDATE packets | `shared/protocol/` | ⏳ Pending |
| 10 — Robot B logs data | `robot_b/` | ⏳ Pending |
| 11 — Robot B reacts | `robot_b/` | ⏳ Pending |
| 12 — Full coordination | Both | ⏳ Pending |

## XBee Current Config (set in XCTU)
- Function Set: ZIGBEE TH PRO
- PAN ID: 3333
- AP: 0 (transparent) → will change to 1 (API) in Phase 8
- Radio A DL: 41F52F20 (points to B)
- Radio B DL: 41F52FD2 (points to A)
- Baud: 9600

> Change AP to 1 on both modules in XCTU before starting Phase 8.
