# esp32-obd

Work in progress — under active development. Features and wiring may change.

A small ESP32-based OBD-II (CAN) reader and adapter.

## Overview

This repository contains firmware for an ESP32 that communicates with a vehicle's CAN/OBD-II network. It provides a simple foundation for reading OBD PIDs and forwarding data over serial or other interfaces.

## Features

- Read OBD-II PIDs over CAN
- Basic CAN adapter implementation in `lib/esp32-obd/`
- PlatformIO-based build system

## Hardware

- ESP32 development board
- CAN transceiver (e.g. SN65HVD230, MCP2551) connected to the ESP32 CAN pins
- OLED display (e.g. SSD1306) connected via I2C for local status and telemetry
- Addressable LED strip (e.g. WS2812) driven from a GPIO for visual feedback

Wiring will depend on your transceiver, display and LED strip — consult their datasheets and match TX/RX, I2C SDA/SCL, power and GND.

## Build & Upload

This project uses PlatformIO. Build and upload with:

```bash
platformio run
platformio run --target upload
```

Open the serial monitor at 9600 baud:

```bash
platformio device monitor -b 9600
```

If you need a specific environment, check `platformio.ini` for environment names.

## Repository Layout

- `src/` - main firmware source (`main.cpp`)
- `lib/esp32-obd/` - library code: `CANAdapter.*`, `OBD.*`
- `include/` - public headers
- `pids.json` - list of supported OBD PIDs

## Usage

1. Build and upload firmware to your ESP32.
2. Connect the CAN transceiver to the vehicle or CAN bus.
3. Use the serial monitor to view OBD data.

## Tests

No automated tests are included. You can add PlatformIO test cases in the `test/` folder.

## License

