// The user specific settings, like pin mappings or special configuration variables and sensitivities are stored in config.h.
// Please open config_sample.h, adjust your settings and save it as config.h
#include "config.h"

#ifdef LEDpin
#include <FastLED.h>

CRGB LED[LEDSnum];
#define MaxLEDbrightness 200

void showled(uint8_t r, uint8_t g, uint8_t b) {
  static uint8_t lastR = 255, lastG = 255, lastB = 255;
  if (r == lastR && g == lastG && b == lastB) return;

  for (int i = 0; i < LEDSnum; i++) {
    LED[i] = CRGB(r, g, b);
  }
  FastLED.show();

  lastR = r;
  lastG = g;
  lastB = b;
}

void fadeWithDelay(int start, int end, int step, int frameDelay = 4) {
  if (step == 0) return;
  for (int i = start; (step > 0) ? i <= end : i >= end; i += step) {
    showled(i, i, i);
    delay(frameDelay);
  }
}

/// @brief Initialize the LED ring. Call this once during setup()
void initLEDring() {
  FastLED.addLeds<WS2811, LEDpin, GRB>(LED, LEDSnum);
  FastLED.setBrightness(MaxLEDbrightness);
  fadeWithDelay(0, MaxLEDbrightness, +1);   // fade in
  fadeWithDelay(MaxLEDbrightness, 50, -1);  // fade out to 50
  fadeWithDelay(0, MaxLEDbrightness, +1);   // fade in
}



void updateLEDsBasedOnMotion(int16_t *velocity, bool State) {
  unsigned long now = millis();
  static unsigned long lastLEDupdate = now;

  if (now - lastLEDupdate >= LEDUPDATERATE_MS) {
    lastLEDupdate = now;
    if (!State) {
      showled(0, 0, 0);
      return;
    }

    int trX = (velocity[TRANSX]) + (-velocity[ROTY]);
    int trY = (velocity[TRANSY]) + (velocity[ROTX]);
    int trZ = (velocity[TRANSZ]) + (-velocity[ROTZ]);


    // “no motion” → white
    if (abs(trX) < 10 && abs(trY) < 10 && abs(trZ) < 10) {
      showled(MaxLEDbrightness, MaxLEDbrightness, MaxLEDbrightness);
      return;
    }

    // Map movement magnitude → intensity
    int mappedX = map(constrain(abs(trX), 0, 200), 0, 200, 0, MaxLEDbrightness);
    int mappedY = map(constrain(abs(trY), 0, 200), 0, 200, 0, MaxLEDbrightness);
    int mappedZ = map(constrain(abs(trZ), 0, 200), 0, 200, 0, MaxLEDbrightness);

    // Z translation → All LEDs fade together
    if (abs(trZ) >= abs(trX) && abs(trZ) >= abs(trY)) {
      showled(MaxLEDbrightness, MaxLEDbrightness - mappedZ, MaxLEDbrightness - mappedZ);
      return;
    }

    CRGB colors[LEDSnum];
    for (int i = 0; i < LEDSnum; i++) colors[i] = CRGB(MaxLEDbrightness, MaxLEDbrightness, MaxLEDbrightness);

    CRGB trXmoveColor = CRGB(MaxLEDbrightness, MaxLEDbrightness - mappedX, MaxLEDbrightness - mappedX);
    CRGB trYmoveColor = CRGB(MaxLEDbrightness, MaxLEDbrightness - mappedY, MaxLEDbrightness - mappedY);

    // X translation
    if (trX < -10) {
      colors[1] = trXmoveColor;
    } else if (trX > 10) {
      colors[3] = trXmoveColor;
    }

    // Y translation
    if (trY < -10) {
      colors[0] = trYmoveColor;
    } else if (trY > 10) {
      colors[2] = trYmoveColor;
    }

    // write out
    for (int i = 0; i < LEDSnum; i++) {
      LED[i] = colors[i];
    }
    FastLED.show();
  }
}

#endif  // #if LEDring
