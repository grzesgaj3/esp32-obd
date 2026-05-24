#include "CANAdapter.h"
#include <SPI.h>
#include <mcp_can.h>

static MCP_CAN* CAN0 = nullptr;

CANAdapter::CANAdapter() : _csPin(5), _intPin(4) {}

bool CANAdapter::begin(int csPin, int intPin, long speed) {
  _csPin = csPin;
  _intPin = intPin;
  if (CAN0) {
    delete CAN0;
    CAN0 = nullptr;
  }
  CAN0 = new MCP_CAN((uint8_t)_csPin);
  // Use SPI pins according to wiring: SCK=IO18, MISO=IO19, MOSI=IO23
  // ensuring MCP2515 is connected to these pins
  SPI.begin(18, 19, 23);
  if (CAN0->begin(MCP_STDEXT, CAN_500KBPS, MCP_8MHZ) == CAN_OK) {
    CAN0->setMode(MCP_NORMAL);
    return true;
  }
  return false;
}

bool CANAdapter::sendRequest(uint32_t id, const uint8_t* data, uint8_t len) {
  if (!CAN0) return false;
  uint8_t res = CAN0->sendMsgBuf(id, 0, len, (uint8_t*)data);
  return (res == CAN_OK);
}

bool CANAdapter::available() {
  if (!CAN0) return false;
  return (CAN0->checkReceive() == CAN_MSGAVAIL);
}

bool CANAdapter::readMessage(uint32_t &id, uint8_t *buf, uint8_t &len) {
  if (!CAN0) return false;
  unsigned long canId = 0;
  uint8_t dlc = 0;
  // readMsgBuf expects (id, len, buf) or (id, ext, len, buf)
  if (CAN0->readMsgBuf(&canId, &dlc, buf) == CAN_OK) {
    id = (uint32_t)canId;
    len = dlc;
    return true;
  }
  return false;
}

void CANAdapter::debug(Print &out) {
  if (!CAN0) {
    out.println("CAN: not initialized");
    return;
  }
  out.print("CAN CS pin: "); out.println(_csPin);
  out.print("CAN INT pin: "); out.println(_intPin);
  out.print("checkReceive(): "); out.println(CAN0->checkReceive());
  out.print("checkError(): "); out.println(CAN0->checkError());
  out.print("getError(): 0x"); out.println(CAN0->getError(), HEX);
  out.print("errorCountRX(): "); out.println(CAN0->errorCountRX());
  out.print("errorCountTX(): "); out.println(CAN0->errorCountTX());
}

bool CANAdapter::selfTestLoopback(Print &out, uint16_t timeout_ms) {
  if (!CAN0) return false;
  out.println("CAN: starting loopback self-test");
  // switch to loopback mode
  if (CAN0->setMode(MCP_LOOPBACK) != MCP2515_OK) {
    out.println("CAN: failed to set LOOPBACK mode");
    return false;
  }

  // send test message
  uint8_t testData[3] = {0xDE, 0xAD, 0xBE};
  if (CAN0->sendMsgBuf(0x123, 0, 3, testData) != CAN_OK) {
    out.println("CAN: sendMsgBuf failed");
    CAN0->setMode(MCP_NORMAL);
    return false;
  }

  unsigned long start = millis();
  bool ok = false;
  while (millis() - start < timeout_ms) {
    if (CAN0->checkReceive() == CAN_MSGAVAIL) {
      unsigned long id = 0;
      uint8_t len = 0;
      uint8_t buf[8];
      if (CAN0->readMsgBuf(&id, &len, buf) == CAN_OK) {
        out.print("CAN: loopback received id=0x"); out.println(id, HEX);
        out.print("CAN: data:");
        for (uint8_t i=0;i<len;i++) { out.print(" 0x"); out.print(buf[i], HEX); }
        out.println();
        // compare first 3 bytes
        if (len >= 3 && buf[0]==testData[0] && buf[1]==testData[1] && buf[2]==testData[2]) ok = true;
        break;
      }
    }
    delay(5);
  }

  CAN0->setMode(MCP_NORMAL);
  if (ok) out.println("CAN: loopback self-test OK"); else out.println("CAN: loopback self-test FAILED");
  return ok;
}
