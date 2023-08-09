#pragma once

#include "scale.hpp"
#include <SPI.h>
#include <U8g2lib.h>

extern const unsigned int SLEEP_AFTER_MS;
extern bool dispAsleep;

// Define for a larger SH1106 OLED display
#define OLED_1_3

#define DISPLAY_RESET_PIN 16
#define DISPLAY_CLOCK_PIN 17
#define DISPLAY_DATA_PIN 18

#define MENU_ITEM_NONE -1
#define MENU_ITEM_MANUAL_GRIND 0
#define MENU_ITEM_CUP_WEIGHT 1
#define MENU_ITEM_CALIBRATE 2
#define MENU_ITEM_OFFSET 3
#define MENU_ITEM_SCALE_MODE 4
#define MENU_ITEM_GRINDING_MODE 5
#define MENU_ITEM_EXIT 6
#define MENU_ITEM_RESET 7

void setupDisplay();
