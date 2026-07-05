# esp32-obd

Firmware for an ESP32-based OBD-II dashboard and CAN adapter.

## Overview

This project targets an ESP32 running the Arduino framework under PlatformIO. It talks to a vehicle CAN bus through an MCP2515-based CAN adapter, requests the OBD-II RPM PID, prints status over serial, and drives an addressable LED bar as a visual tachometer.

The current firmware is focused on one live telemetry path: PID `0x0C` for engine RPM. It also performs a CAN controller diagnostic on boot and runs a loopback self-test so wiring and controller state are easier to verify.

## What It Does

- Initializes the CAN adapter and checks for a working link
- Requests OBD-II RPM data over CAN
- Prints RPM and sync state to the serial monitor at 9600 baud
- Maps RPM to a 21-LED bar with a blue-to-red gradient
- Flashes the LED bar red when RPM stays at redline briefly
- Runs a startup LED animation to confirm the strip is wired correctly

## Hardware

The firmware currently assumes:

- An ESP32-C3 class board configured in `platformio.ini`
- An MCP2515-based CAN controller on SPI
- A CAN transceiver connected between the MCP2515 and the vehicle bus
- A WS2815-compatible LED strip or similar addressable LED bar

Current pin assignments are defined in `src/main.cpp`:

- CAN CS: GPIO 21
- CAN INT: GPIO 8
- LED data: GPIO 0

If you change the board or wiring, update those values in `src/main.cpp`.

## Build And Upload

Use PlatformIO to build and flash the firmware:

```bash
platformio run
platformio run --target upload
```

To watch the serial output:

```bash
platformio device monitor -b 9600
```

## Project Layout

- `src/main.cpp` - application entry point, CAN polling, and LED logic
- `lib/esp32-obd/OBD.*` - OBD request/response handling
- `lib/esp32-obd/CANAdapter.*` - MCP2515 CAN controller wrapper
- `include/` - public headers
- `pids.json` - PID metadata list

## Notes

This repository is still work in progress. The wiring, supported PIDs, and display behavior may change as the firmware grows beyond the current RPM-focused dashboard.

## Tests

No automated tests are included yet. PlatformIO test cases can be added under `test/` if needed.

## License

No license has been specified yet.

