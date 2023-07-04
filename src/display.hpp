#pragma once

#include "scale.hpp"
#include <SPI.h>
#include <U8g2lib.h>

extern const unsigned int SLEEP_AFTER_MS;
extern bool dispAsleep;

#define DISPLAY_RESET_PIN 16
#define DISPLAY_CLOCK_PIN 17
#define DISPLAY_DATA_PIN 18

void setupDisplay();
