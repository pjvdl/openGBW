#pragma once

#include "scale.hpp"
#include <SPI.h>
#include <U8g2lib.h>

extern const unsigned int SLEEP_AFTER_MS;
extern bool dispAsleep;

void setupDisplay();
