#include "OBD.h"
#include <Arduino.h>

OBD::OBD() : _lastPID(0), _lastResponseTs(0), _rpm(0), _synced(false), _lastRequestTs(0) {}

bool OBD::begin(int csPin, int intPin) {
  bool ok = _can.begin(csPin, intPin);
  // initialize response timers after CAN is started
  _lastResponseTs = millis();
  _lastRequestTs = millis();
  return ok;
}

void OBD::requestPID(uint8_t pid) {
  // Functional request to 0x7DF
  uint8_t data[8] = {0};
  data[0] = 0x02; // number of additional bytes
  data[1] = 0x01; // service 01
  data[2] = pid;  // PID
  for (int i = 3; i < 8; i++) data[i] = 0x00;
  _can.sendRequest(0x7DF, data, 8);
  _lastPID = pid;
  _lastRequestTs = millis();
}

void OBD::poll() {
  // check for incoming CAN messages
  uint32_t id;
  uint8_t buf[8];
  uint8_t len = 0;
  while (_can.available()) {
    if (_can.readMessage(id, buf, len)) {
      // typical ECU response IDs are 0x7E8..0x7EF
      if (id >= 0x7E8 && id <= 0x7EF && len >= 4) {
        // expect [N, 0x41, PID, A, B...]
        if (buf[1] == 0x41 && buf[2] == _lastPID) {
          if (_lastPID == 0x0C) { // RPM
            uint16_t A = buf[3];
            uint16_t B = buf[4];
            _rpm = ((A << 8) | B) / 4;
            _synced = true;
            _lastResponseTs = millis();
          } else {
            // other PIDs can be parsed here
            _synced = true;
            _lastResponseTs = millis();
          }
        }
      }
    }
  }

  // if no response for >1500ms mark not synced
  if (millis() - _lastResponseTs > 1500) {
    _synced = false;
  }
}

uint16_t OBD::getRPM() const { return _rpm; }

bool OBD::isSynced() const { return _synced; }

void OBD::debug(Print &out) { _can.debug(out); }

bool OBD::selfTestLoopback(Print &out, uint16_t timeout_ms) { return _can.selfTestLoopback(out, timeout_ms); }
