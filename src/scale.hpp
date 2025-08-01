#pragma once

#include <SimpleKalmanFilter.h>
#include "HX711.h"
#include "display.hpp"

class MenuItem
{
    public:
        int id;
        bool selected;
        char menuName[16];
        double increment;
        double *value;
};

#define STATUS_EMPTY 0
#define STATUS_GRINDING_IN_PROGRESS 1
#define STATUS_GRINDING_FINISHED 2
#define STATUS_GRINDING_FAILED 3
#define STATUS_IN_MENU 4
#define STATUS_IN_SUBMENU 5
#define STATUS_IN_SUBSUBMENU 6
#define STATUS_TARING 7
#define STATUS_MANUAL_READY 8
#define STATUS_MANUAL_IN_PROGRESS 9
#define STATUS_OFFSET_WARNING 10

#define WARNING_ON_OFFSET 1

#define CUP_WEIGHT 574
#define CUP_DETECTION_TOLERANCE 10 // 10 grams tolerance above or bellow cup weight to detect it
#define WAKE_UP_WEIGHT_TOLERANCE 300 // 300 grams tolerance to wake up display

#define EXPECTED_OFFSET -1.0 // expected offset for this grinder

#define LOADCELL_DOUT_PIN 6
#define LOADCELL_SCK_PIN 7

#define LOADCELL_SCALE_FACTOR -657
#define LOADCELL_NUMBER_READINGS 5

#define TARE_MEASURES 20 // use the average of measure for taring
#define SIGNIFICANT_WEIGHT_CHANGE 20 // 50 grams changes are used to detect a significant change
#define COFFEE_DOSE_WEIGHT 18
#define COFFEE_DOSE_OFFSET -1.0
#define MAX_GRINDING_TIME 30000 // 30 seconds diff
#define GRINDING_DELAY_TOLERANCE 5000 // 5 seconds
#define GRINDING_FAILED_WEIGHT_TO_RESET 150 // force on balance need to be measured to reset grinding

#define GRINDER_ACTIVE_PIN 14

#define TARE_MIN_INTERVAL 10 * 1000 // auto-tare at most once every 60 seconds

#define ROTARY_ENCODER_A_PIN 40
#define ROTARY_ENCODER_B_PIN 41
#define ROTARY_ENCODER_BUTTON_PIN 42
#define ROTARY_ENCODER_VCC_PIN -1
#define ROTARY_ENCODER_STEPS 4

extern double scaleWeight;
extern double calibrationFactor;
extern bool wakeDisp;
extern unsigned long scaleLastUpdatedAt;
extern unsigned long lastAction;
extern unsigned long lastTareAt;
extern bool scaleReady;
extern bool calibrationError;
extern int scaleStatus;
extern double cupWeightEmpty;
extern unsigned long startedGrindingAt;
extern unsigned long finishedGrindingAt;
extern double setWeight;
extern double offset;
extern bool scaleMode;
extern bool grindMode;
extern bool greset;
extern int menuItemsCount;

extern MenuItem menuItems[];
extern int currentMenuItem;
extern int currentSetting;

void calibrateScale();
void setupScale();
void printPreferencesToSerial();