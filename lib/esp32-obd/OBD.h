#pragma once
#include <Arduino.h>
#include "CANAdapter.h"

class OBD {
public:
  OBD();
  bool begin(int csPin = 21, int intPin = 8);
  void requestPID(uint8_t pid);
  void poll();
  uint16_t getRPM() const;
  bool isSynced() const;
  void debug(Print &out);
  bool selfTestLoopback(Print &out, uint16_t timeout_ms = 200);
private:
  CANAdapter _can;
  uint8_t _lastPID;
  unsigned long _lastResponseTs;
  uint16_t _rpm;
  bool _synced;
  unsigned long _lastRequestTs;
};
