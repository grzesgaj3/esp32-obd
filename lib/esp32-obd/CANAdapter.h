#pragma once
#include <Arduino.h>

class CANAdapter {
public:
  CANAdapter();
  bool begin(int csPin, int intPin, long speed = 500000);
  bool sendRequest(uint32_t id, const uint8_t* data, uint8_t len);
  bool available();
  bool readMessage(uint32_t &id, uint8_t *buf, uint8_t &len);
  // Print debug information about the MCP2515 / CAN controller
  void debug(Print &out);
  // Run a loopback self-test on the MCP2515 (switches to LOOPBACK mode briefly)
  bool selfTestLoopback(Print &out, uint16_t timeout_ms = 200);
private:
  int _csPin;
  int _intPin;
};
