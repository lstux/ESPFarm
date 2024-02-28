#ifndef _display_h_
#define _display_h

#include <Arduino.h>

enum display_mode {
  DISPLAY_CURRENT,
  DISPLAY_MAX,
  DISPLAY_MIN,
  DISPLAY_AVG
};

bool display_setup();
void display_bootup(const char *label="", uint8_t pct=100, uint16_t delayms=0);
void display_main(enum display_mode mode=DISPLAY_CURRENT);

#endif
