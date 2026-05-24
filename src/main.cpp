#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "OBD.h"
#include <Adafruit_NeoPixel.h>

// Pins per wiring: CS -> IO5, INT -> IO4 (see provided mapping)
const int CAN_CS_PIN = 5;
const int CAN_INT_PIN = 4;

// SSD1306 configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// Addressable LED bar (configurable)
const int LED_PIN = 27; // change this to the pin you wired the LED strip to
const int NUM_LEDS = 10; // bar length
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

OBD obd;
// Configurable intervals (ms)
unsigned long lastRequest = 0;
unsigned long lastDisplay = 0;
const unsigned long RPM_REQUEST_MIN_INTERVAL_MS = 100; // request RPM every 100ms (10 Hz)
const unsigned long RPM_REQUEST_BACKOFF_MS = 500; // backoff interval when no response
const unsigned long DISPLAY_REFRESH_MS = 100; // update display every 100ms (10 Hz)
unsigned long currentRequestInterval = RPM_REQUEST_MIN_INTERVAL_MS;

// RPM -> LED mapping
const uint16_t RPM_MAX = 9000; // top of scale

// Map RPM to LED bar and set colors (green -> yellow -> red) using NeoPixel
static void hsvToRgb(uint8_t h, uint8_t s, uint8_t v, uint8_t &r, uint8_t &g, uint8_t &b) {
  float hf = h / 255.0f * 360.0f;
  float sf = s / 255.0f;
  float vf = v / 255.0f;
  int hi = (int)(hf / 60.0f) % 6;
  float f = (hf / 60.0f) - floor(hf / 60.0f);
  float p = vf * (1.0f - sf);
  float q = vf * (1.0f - f * sf);
  float t = vf * (1.0f - (1.0f - f) * sf);
  float rr, gg, bb;
  switch (hi) {
    case 0: rr = vf; gg = t;  bb = p; break;
    case 1: rr = q;  gg = vf; bb = p; break;
    case 2: rr = p;  gg = vf; bb = t; break;
    case 3: rr = p;  gg = q;  bb = vf; break;
    case 4: rr = t;  gg = p;  bb = vf; break;
    default: rr = vf; gg = p; bb = q; break;
  }
  r = (uint8_t)(rr * 255.0f);
  g = (uint8_t)(gg * 255.0f);
  b = (uint8_t)(bb * 255.0f);
}

// Animation state
float filteredRpm = 0.0f;
const float RPM_SMOOTH_ALPHA = 0.25f; // 0..1, higher = less smoothing

// Peak indicator
int peakLed = 0;
unsigned long peakTs = 0;
const unsigned long PEAK_HOLD_MS = 300;
const unsigned long PEAK_DECAY_MS = 800;

unsigned long lastLedAnim = 0;

void updateLedBar(uint16_t rpm) {
  unsigned long now = millis();
  // smooth RPM for pleasant animation
  filteredRpm = filteredRpm * (1.0f - RPM_SMOOTH_ALPHA) + ((float)rpm) * RPM_SMOOTH_ALPHA;

  float ratio = (float)filteredRpm / (float)RPM_MAX;
  if (ratio < 0) ratio = 0;
  if (ratio > 1) ratio = 1;
  float scaled = ratio * NUM_LEDS;
  int lit = (int)floor(scaled);
  float partial = scaled - lit;

  // update peak indicator
  if (lit > peakLed) {
    peakLed = lit;
    peakTs = now;
  }

  // animate LEDs with a comet/trail effect
  const float TRAIL_DECAY = 0.55f; // per-step brightness decay
  for (int i = 0; i < NUM_LEDS; ++i) {
    float brightness = 0.0f;
    if (i < lit) brightness = 1.0f; // fully lit
    else if (i == lit) brightness = partial; // partial fill
    else brightness = 0.0f;

    // apply trail: closer to lit index -> brighter
    int distance = lit - i;
    if (distance > 0) brightness *= powf(TRAIL_DECAY, (float)distance);

    // peak indicator overlay (bright dot that holds briefly)
    if (peakLed > 0) {
      if ((now - peakTs) < PEAK_HOLD_MS) {
        if (i == peakLed - 1) brightness = max(brightness, 1.0f);
      } else {
        // decay after hold
        unsigned long dec = now - (peakTs + PEAK_HOLD_MS);
        if (dec < PEAK_DECAY_MS) {
          float decayFactor = 1.0f - (float)dec / (float)PEAK_DECAY_MS;
          if (i == peakLed - 1) brightness = max(brightness, decayFactor);
        } else {
          peakLed = max(0, peakLed - 1);
          peakTs = now;
        }
      }
    }

    // idle breathing effect when RPM==0
    if ((int)filteredRpm == 0) {
      float breath = 0.5f + 0.5f * sinf((now % 2000) / 2000.0f * 2.0f * 3.14159f);
      brightness *= breath * (i==0 ? 1.0f : 0.45f);
    }

    // compute hue shifting along the bar for visual richness
    uint8_t baseHue = (uint8_t)map((int)filteredRpm, 0, RPM_MAX, 96, 0);
    int hueShift = i * 6; // subtle gradient
    uint8_t hue = (uint8_t)max(0, min(255, (int)baseHue - hueShift));
    uint8_t val = (uint8_t)min(255, (int)(brightness * 255.0f));

    uint8_t r,g,b;
    hsvToRgb(hue, 220, val, r, g, b);
    strip.setPixelColor(i, strip.Color(r, g, b));
  }
  strip.show();
}
bool displayOk = false;

void setup() {
  Serial.begin(9600);
  delay(100);
  Serial.println("Starting ESP32 OBD display");

  // initialize I2C (common ESP32 pins SDA=21, SCL=22)
  Wire.begin(22, 21);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 init failed");
  } else {
    displayOk = true;
    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SSD1306 OK");
    display.display();
  }

  // init NeoPixel strip
  strip.begin();
  strip.setBrightness(180);
  strip.show();

  if (!obd.begin(CAN_CS_PIN, CAN_INT_PIN)) {
    Serial.println("CAN init failed");
  } else {
    Serial.println("CAN init OK");
  }

  // Print CAN/MCP diagnostics
  Serial.println("--- CAN diagnostics ---");
  obd.debug(Serial);
  Serial.println("--- CAN self-test (loopback) ---");
  bool loopOk = obd.selfTestLoopback(Serial, 300);
  Serial.print("Loopback result: "); Serial.println(loopOk ? "OK" : "FAILED");

  // initial PID request
  obd.requestPID(0x0C);
  lastRequest = millis();
}

void loop() {
  obd.poll();

  unsigned long now = millis();
  // adapt request interval: try fast when synced, backoff when not
  if (obd.isSynced()) currentRequestInterval = RPM_REQUEST_MIN_INTERVAL_MS;
  else currentRequestInterval = RPM_REQUEST_BACKOFF_MS;

  if (now - lastRequest > currentRequestInterval) {
    obd.requestPID(0x0C);
    lastRequest = now;
    Serial.print("Requested PID 0x0C @ "); Serial.print(millis()); Serial.print(" ms (interval "); Serial.print(currentRequestInterval); Serial.println(" ms)");
  }

  if (now - lastDisplay > 250) {
    lastDisplay = now;
    uint16_t rpm = obd.getRPM();
    if (displayOk) {
      display.clearDisplay();
      display.setTextSize(2);
      display.setCursor(0, 0);
      if (rpm == 0) display.print("---"); else display.print(rpm);
      display.print(" RPM");

      display.setTextSize(1);
      display.setCursor(0, 24);
      if (obd.isSynced()) display.print("SYNC"); else display.print("NOT SYNC");
      display.display();
      // update LED bar to reflect RPM
      updateLedBar(rpm);
    } else {
      // fallback to serial output if display not available
      Serial.print("RPM: ");
      Serial.print(rpm);
      Serial.print("  ");
      Serial.println(obd.isSynced() ? "SYNC" : "NOT SYNC");
    }
  }
}