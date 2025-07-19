#include "scale.hpp"
#include <MathBuffer.h>
#include <AiEsp32RotaryEncoder.h>
#include <Preferences.h>

HX711 loadcell;
SimpleKalmanFilter kalmanFilter(0.02, 0.02, 0.01);

Preferences preferences;


AiEsp32RotaryEncoder rotaryEncoder = AiEsp32RotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN, ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

#define ABS(a) (((a) > 0) ? (a) : ((a) * -1))

TaskHandle_t ScaleTask;
TaskHandle_t ScaleStatusTask;

double scaleWeight = 0; //current weight
double calibrationFactor = 1; // current calibration of scale
bool wakeDisp = false; //wake up display with rotary click
double setWeight = 0; //desired amount of coffee
double setCupWeight = 0; //cup weight set by user
double offset = 0; //stop x grams prios to set weight
bool scaleMode = false; //use as regular scale with timer if true
bool grindMode = true;  //false for impulse to start/stop grinding, true for continuous on while grinding
bool grinderActive = false; //needed for continuous mode
MathBuffer<double, 100> weightHistory;

unsigned long lastAction = 0;
unsigned long scaleLastUpdatedAt = 0;
unsigned long lastTareAt = 0; // if 0, should tare load cell, else represent when it was last tared
bool scaleReady = false;
bool calibrationError = false;
int scaleStatus = STATUS_EMPTY;
double cupWeightEmpty = 0; //measured actual cup weight
unsigned long startedGrindingAt = 0;
unsigned long finishedGrindingAt = 0;
int encoderDir = -1;
bool greset = false;

bool newOffset = false;

int currentMenuItem = 0;
int currentSetting;
int encoderValue = 0;
int menuItemsCount = 8;
MenuItem menuItems[8] = {
    {MENU_ITEM_MANUAL_GRIND, false, "Manual Grind", 0},
    {MENU_ITEM_CUP_WEIGHT, false, "Cup weight", 1, &setCupWeight},
    {MENU_ITEM_CALIBRATE, false, "Calibrate", 0},
    {MENU_ITEM_OFFSET, false, "Offset", 0.1, &offset},
    {MENU_ITEM_SCALE_MODE, false, "Scale Mode", 0},
    {MENU_ITEM_GRINDING_MODE, false, "Grinding Mode", 0},
    {MENU_ITEM_EXIT, false, "Exit", 0},
    {MENU_ITEM_RESET, false, "Reset", 0}}; // structure is mostly useless for now, plan on making menu easier to customize later

void grinderToggle()
{
  if(!scaleMode){
    if(grindMode){
      grinderActive = !grinderActive;
      digitalWrite(GRINDER_ACTIVE_PIN, grinderActive);
    }
    else{
      digitalWrite(GRINDER_ACTIVE_PIN, 1);
      delay(100);
      digitalWrite(GRINDER_ACTIVE_PIN, 0);
    }
  }
}


void rotary_onButtonClick()
{
  wakeDisp = 1;
  lastAction = millis();

  if(dispAsleep)
    return;

  if(scaleStatus == STATUS_EMPTY){
    scaleStatus = STATUS_IN_MENU;
    currentMenuItem = MENU_ITEM_MANUAL_GRIND;
    rotaryEncoder.setAcceleration(0);
  }
  else if(scaleStatus == STATUS_OFFSET_WARNING){
    scaleStatus = STATUS_IN_SUBMENU;
    currentSetting = MENU_ITEM_OFFSET;
    Serial.println("Offset Menu from Warning");
    rotaryEncoder.setAcceleration(0);
  }
  else if(scaleStatus == STATUS_MANUAL_IN_PROGRESS) {
    grinderToggle();
    scaleStatus = STATUS_GRINDING_FINISHED;
    finishedGrindingAt = millis();
    Serial.println("Finished Grinding");
  }
  else if(scaleStatus == STATUS_IN_MENU){
    if(currentMenuItem == MENU_ITEM_EXIT){
      scaleStatus = STATUS_EMPTY;
      rotaryEncoder.setAcceleration(100);
      Serial.println("Exited Menu");
    }
    else if (currentMenuItem == MENU_ITEM_MANUAL_GRIND)
    {
      // grinderToggle();
      scaleStatus = STATUS_MANUAL_READY;
      currentSetting = MENU_ITEM_MANUAL_GRIND;
      Serial.println("Manual Grind Menu");
    }
    else if (currentMenuItem == MENU_ITEM_OFFSET){
      scaleStatus = STATUS_IN_SUBMENU;
      currentSetting = MENU_ITEM_OFFSET;
      Serial.println("Offset Menu");
    }
    else if (currentMenuItem == MENU_ITEM_CUP_WEIGHT)
    {
      scaleStatus = STATUS_IN_SUBMENU;
      currentSetting = MENU_ITEM_CUP_WEIGHT;
      Serial.println("Cup Menu");
    }
    else if (currentMenuItem == MENU_ITEM_CALIBRATE)
    {
      scaleStatus = STATUS_IN_SUBMENU;
      currentSetting = MENU_ITEM_CALIBRATE;
      Serial.println("Calibration Menu");
      printPreferencesToSerial();
    }
    else if (currentMenuItem == MENU_ITEM_SCALE_MODE)
    {
      scaleStatus = STATUS_IN_SUBMENU;
      currentSetting = MENU_ITEM_SCALE_MODE;
      Serial.println("Scale Mode Menu");
    }
    else if (currentMenuItem == MENU_ITEM_GRINDING_MODE)
    {
      scaleStatus = STATUS_IN_SUBMENU;
      currentSetting = MENU_ITEM_GRINDING_MODE;
      Serial.println("Grind Mode Menu");
    }
    else if (currentMenuItem == MENU_ITEM_RESET)
    {
      scaleStatus = STATUS_IN_SUBMENU;
      currentSetting = MENU_ITEM_RESET;
      greset = false;
      Serial.println("Reset Menu");
      printPreferencesToSerial();
    }
  }
  else if(scaleStatus == STATUS_IN_SUBMENU){
    if(currentSetting == MENU_ITEM_OFFSET){

      preferences.begin("scale", false);
      preferences.putDouble("offset", offset);
      preferences.end();
      scaleStatus = STATUS_IN_MENU;
      currentSetting = MENU_ITEM_NONE;
    }
    else if (currentSetting == MENU_ITEM_CUP_WEIGHT)
    {
      if(scaleWeight > 30){       //prevent accidental setting with no cup
        setCupWeight = scaleWeight;
        Serial.println(setCupWeight);
        
        preferences.begin("scale", false);
        preferences.putDouble("cup", setCupWeight);
        preferences.end();
        scaleStatus = STATUS_IN_MENU;
        currentSetting = MENU_ITEM_NONE;
      }
    }
    else if (currentSetting == MENU_ITEM_CALIBRATE)
    {
      calibrateScale();
      scaleStatus = STATUS_IN_MENU;
      currentSetting = MENU_ITEM_NONE;
    }
    else if (currentSetting == MENU_ITEM_SCALE_MODE)
    {
      preferences.begin("scale", false);
      preferences.putBool("scaleMode", scaleMode);
      preferences.end();
      scaleStatus = STATUS_IN_MENU;
      currentSetting = MENU_ITEM_NONE;
    }
    else if (currentSetting == MENU_ITEM_GRINDING_MODE)
    {
      preferences.begin("scale", false);
      preferences.putBool("grindMode", grindMode);
      preferences.end();
      scaleStatus = STATUS_IN_MENU;
      currentSetting = MENU_ITEM_NONE;
    }
    else if (currentSetting == MENU_ITEM_RESET)
    {
      if(greset){
        preferences.begin("scale", false);
        preferences.putDouble("calibration", (double)LOADCELL_SCALE_FACTOR);
        setWeight = (double)COFFEE_DOSE_WEIGHT;
        preferences.putDouble("setWeight", (double)COFFEE_DOSE_WEIGHT);
        offset = (double)COFFEE_DOSE_OFFSET;
        preferences.putDouble("offset", (double)COFFEE_DOSE_OFFSET);
        setCupWeight = (double)CUP_WEIGHT;
        preferences.putDouble("cup", (double)CUP_WEIGHT);
        scaleMode = false;
        preferences.putBool("scaleMode", false);
        grindMode = true;
        preferences.putBool("grindMode", true);

        calibrationFactor = (double(LOADCELL_SCALE_FACTOR));
        loadcell.set_scale(calibrationFactor);
        preferences.end();
      }
      
      scaleStatus = STATUS_IN_MENU;
      currentSetting = MENU_ITEM_NONE;
    }
  }
}



void rotary_loop()
{
  if (rotaryEncoder.encoderChanged())
  {
    wakeDisp = 1;
    lastAction = millis();

    if(dispAsleep)
      return;
    
    if(scaleStatus == STATUS_EMPTY){
        int newValue = rotaryEncoder.readEncoder();
        Serial.print("Value: ");

        setWeight += ((float)newValue - (float)encoderValue) / 10 * encoderDir;

        encoderValue = newValue;
        Serial.println(newValue);
        preferences.begin("scale", false);
        preferences.putDouble("setWeight", setWeight);
        preferences.end();
    }
    else if(scaleStatus == STATUS_IN_MENU){
      int newValue = rotaryEncoder.readEncoder();
      currentMenuItem = (currentMenuItem + (newValue - encoderValue) * encoderDir) % menuItemsCount;
      currentMenuItem = currentMenuItem < 0 ? menuItemsCount + currentMenuItem : currentMenuItem;
      encoderValue = newValue;
      Serial.println(currentMenuItem);
    }
    else if(scaleStatus == STATUS_IN_SUBMENU){
      if(currentSetting == MENU_ITEM_OFFSET){ //offset menu
        int newValue = rotaryEncoder.readEncoder();
        Serial.print("Value: ");

        offset += ((float)newValue - (float)encoderValue) * encoderDir / 10;
        encoderValue = newValue;

        if(abs(offset) >= setWeight){
          offset = setWeight;     //prevent nonsensical offsets
        }
      }
      else if(currentSetting == MENU_ITEM_SCALE_MODE){
        scaleMode = !scaleMode;
      }
      else if (currentSetting == MENU_ITEM_GRINDING_MODE)
      {
        grindMode = !grindMode;
      }
      else if (currentSetting == MENU_ITEM_RESET)
      {
        greset = !greset;
      }
    }
  }
  if (rotaryEncoder.isEncoderButtonClicked())
  {
    rotary_onButtonClick();
  }
  if (wakeDisp && ((millis() - lastAction) > 1000) )
    wakeDisp = 0;
}

void readEncoderISR()
{
  rotaryEncoder.readEncoder_ISR();
}

void tareScale() {
  Serial.println("Taring scale");
  loadcell.tare(TARE_MEASURES);
  lastTareAt = millis();
}

void calibrateScale() {
  preferences.begin("scale", false);
  double existingCalibrationFactor = preferences.getDouble("calibration");
  calibrationFactor = existingCalibrationFactor * (scaleWeight / 100);
  Serial.printf("New calibration: %f. Existing calibration: %f, Scale weight: %f\n", calibrationFactor, existingCalibrationFactor, scaleWeight);
  if (isnan(calibrationFactor) || calibrationFactor == 0.0) {
    Serial.println("Resetting calibration because weight is NaN or 0.0");
    calibrationFactor = (double(LOADCELL_SCALE_FACTOR));
    calibrationError = true;
  }
  else {
    calibrationError = false;
    preferences.putDouble("calibration", calibrationFactor);
  }
  loadcell.set_scale(calibrationFactor);
  preferences.end();
}

void updateScale( void * parameter) {
  float lastEstimate;

  for (;;) {
    if (lastTareAt == 0) {
      Serial.println("retaring scale");
      Serial.println("current offset");
      Serial.println(offset);
      tareScale();
    }
    if (loadcell.wait_ready_timeout(300)) {
      lastEstimate = kalmanFilter.updateEstimate(loadcell.get_units(LOADCELL_NUMBER_READINGS)); // Update the kalman filter, but don't use the value
      scaleWeight = lastEstimate;
      scaleLastUpdatedAt = millis();
      weightHistory.push(scaleWeight);
      scaleReady = true;
    } else {
      Serial.println("HX711 not found.");
      scaleReady = false;
    }
  }
}

void scaleStatusLoop(void *p) {
  double tenSecAvg;
  bool significantWeightChange;

  for (;;) {
    tenSecAvg = weightHistory.averageSince((int64_t)millis() - 10000);

    significantWeightChange = ABS(tenSecAvg - scaleWeight) > SIGNIFICANT_WEIGHT_CHANGE;
    if (significantWeightChange) {
      lastAction = millis();
    }

    if (scaleStatus == STATUS_EMPTY) {
      // if scale is empty, check if it should be tared
      if (((millis() - lastTareAt) > TARE_MIN_INTERVAL)
          && (ABS(tenSecAvg) >= 0.1)
          && (tenSecAvg < 10) // if scale is not empty, tare it
          && (!significantWeightChange)) {
        // tare if: not tared recently, more than 0.2 away from 0, less than 3 grams total (also works for negative weight)
        lastTareAt = 0;
        scaleStatus = STATUS_TARING;
        wakeDisp = 1;
        Serial.println("Taring scale");
      }

      // if the offset has changed outside of the tolerance for this grinder, show warning
      if (ABS(offset - EXPECTED_OFFSET) >= 0.3 && WARNING_ON_OFFSET) {
        scaleStatus = STATUS_OFFSET_WARNING;
        Serial.printf("Offset warning, expected: %f, actual: %f\n", EXPECTED_OFFSET, offset);
        wakeDisp = 1;
        continue;
      }

      // wake up display if a weight is detected on the scale
      if (ABS(weightHistory.minSince((int64_t)millis() - 1000)) > WAKE_UP_WEIGHT_TOLERANCE 
        && ABS(weightHistory.maxSince((int64_t)millis() - 1000)) > WAKE_UP_WEIGHT_TOLERANCE
          && (lastTareAt != 0)
          && scaleReady)
      {
        wakeDisp = 1;
      }

      // if the weight is within tolerance of the cup weight, start grinding
      if (ABS(weightHistory.minSince((int64_t)millis() - 1000) - setCupWeight) < CUP_DETECTION_TOLERANCE 
        && ABS(weightHistory.maxSince((int64_t)millis() - 1000) - setCupWeight) < CUP_DETECTION_TOLERANCE
        && (lastTareAt != 0)
        && scaleReady)
      {
        // using average over last 500ms as empty cup weight
        wakeDisp = 1;
        Serial.println("Starting grinding");
        cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
        scaleStatus = STATUS_GRINDING_IN_PROGRESS;
        
        if(!scaleMode){
          newOffset = true;
          startedGrindingAt = millis();
        }
        
        grinderToggle();
        continue;
      }
    } else if (scaleStatus == STATUS_MANUAL_READY) {
      Serial.println("Starting grinding");
      cupWeightEmpty = weightHistory.averageSince((int64_t)millis() - 500);
      scaleStatus = STATUS_MANUAL_IN_PROGRESS;
      
      if(!scaleMode){
        newOffset = true;
        startedGrindingAt = millis();
      }
      
      grinderToggle();
      continue;

    } else if (scaleStatus == STATUS_GRINDING_IN_PROGRESS || scaleStatus == STATUS_MANUAL_IN_PROGRESS) {
      if (!scaleReady) {
        
        grinderToggle();
        scaleStatus = STATUS_GRINDING_FAILED;
      }
      //Serial.printf("Scale mode: %d\n", scaleMode);
      //Serial.printf("Started grinding at: %d\n", startedGrindingAt);
      //Serial.printf("Weight: %f\n", cupWeightEmpty - scaleWeight);
      if (scaleMode && (startedGrindingAt == 0) && ((scaleWeight - cupWeightEmpty) >= 0.1))
      {
        Serial.printf("Started grinding at: %d\n", millis());
        startedGrindingAt = millis();
        continue;
      }

      if (((millis() - startedGrindingAt) > MAX_GRINDING_TIME) && !scaleMode) {
        Serial.println("Failed because grinding took too long");
        
        grinderToggle();
        scaleStatus = STATUS_GRINDING_FAILED;
        continue;
      }

      if ( ((millis() - startedGrindingAt) > GRINDING_DELAY_TOLERANCE) // started grinding at least 3s ago
            && ((scaleWeight - weightHistory.firstValueOlderThan(millis() - 2000)) < 0.5) // less than a gram has been grinded in the last 2 second
            && !scaleMode) {
        Serial.println("Failed because no change in weight was detected");
        
        grinderToggle();
        scaleStatus = STATUS_GRINDING_FAILED;
        continue;
      }

      // if (weightHistory.minSince((int64_t)millis() - 200) < (cupWeightEmpty - CUP_DETECTION_TOLERANCE) 
      //       && !scaleMode) {
      //   Serial.printf("Failed because weight too low, min: %f, min value: %f\n", weightHistory.minSince((int64_t)millis() - 200), CUP_WEIGHT + CUP_DETECTION_TOLERANCE);
        
      //   grinderToggle();
      //   scaleStatus = STATUS_GRINDING_FAILED;
      //   continue;
      // }
      double currentOffset = offset;
      if(scaleMode){
        currentOffset = 0;
      }

      if (weightHistory.maxSince((int64_t)millis() - 200) >= (cupWeightEmpty + setWeight + currentOffset)) {
        Serial.println("Finished grinding");
        finishedGrindingAt = millis();
        
        grinderToggle();
        scaleStatus = STATUS_GRINDING_FINISHED;
        continue;
      }
    } else if (scaleStatus == STATUS_GRINDING_FINISHED) {
      double currentWeight = weightHistory.averageSince((int64_t)millis() - 500);
      if (scaleWeight < 5) {
        Serial.println("Going back to empty");
        startedGrindingAt = 0;
        scaleStatus = STATUS_EMPTY;
        continue;
      }
      else if ((currentWeight != (setWeight + cupWeightEmpty)) 
                && ((millis() - finishedGrindingAt) > 1500)
                && newOffset)
      {
        offset += (setWeight + cupWeightEmpty - currentWeight);
        if(ABS(offset) >= setWeight){
          offset = COFFEE_DOSE_OFFSET;
        }
        preferences.begin("scale", false);
        preferences.putDouble("offset", offset);
        preferences.end();
        newOffset = false;
      }
    } else if (scaleStatus == STATUS_GRINDING_FAILED) {
      // Will reset when the portafilter is removed from the scale
      if (scaleWeight < GRINDING_FAILED_WEIGHT_TO_RESET) {
        Serial.println("Going back to empty");
        scaleStatus = STATUS_EMPTY;
        continue;
      }
    } else if (scaleStatus == STATUS_TARING) {
      if (lastTareAt != 0) {
        scaleStatus = STATUS_EMPTY;
      }
    } else if (scaleStatus == STATUS_OFFSET_WARNING) {
      wakeDisp = 1;
    }
    rotary_loop();
    delay(50);
  }
}


void printPreferencesToSerial() {
        preferences.begin("scale", true);
        Serial.printf("Calibration: %f\n", preferences.getDouble("calibration"));
        Serial.printf("Dose weight: %f\n", preferences.getDouble("setWeight"));
        Serial.printf("Offset time: %f\n", preferences.getDouble("offset"));
        Serial.printf("Cup weight: %f\n", preferences.getDouble("cup"));
        Serial.printf("Scale mode: %d\n", preferences.getBool("scaleMode"));
        Serial.printf("Grind mode: %d\n", preferences.getBool("grindMode"));
        preferences.end();  
}

void setupScale() {
  rotaryEncoder.begin();
  rotaryEncoder.setup(readEncoderISR);
  // set boundaries and if values should cycle or not
  // in this example we will set possible values between 0 and 1000;
  bool circleValues = true;
  rotaryEncoder.setBoundaries(-10000, 10000, circleValues); // minValue, maxValue, circleValues true|false (when max go to min and vice versa)

  /*Rotary acceleration introduced 25.2.2021.
   * in case range to select is huge, for example - select a value between 0 and 1000 and we want 785
   * without accelerateion you need long time to get to that number
   * Using acceleration, faster you turn, faster will the value raise.
   * For fine tuning slow down.
   */
  // rotaryEncoder.disableAcceleration(); //acceleration is now enabled by default - disable if you dont need it
  rotaryEncoder.setAcceleration(100); // or set the value - larger number = more accelearation; 0 or 1 means disabled acceleration


  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  pinMode(GRINDER_ACTIVE_PIN, OUTPUT);
  digitalWrite(GRINDER_ACTIVE_PIN, 0);

  preferences.begin("scale", true);
  
  calibrationFactor = preferences.getDouble("calibration", (double)LOADCELL_SCALE_FACTOR);
  setWeight = preferences.getDouble("setWeight", (double)COFFEE_DOSE_WEIGHT);
  offset = preferences.getDouble("offset", (double)COFFEE_DOSE_OFFSET);
  setCupWeight = preferences.getDouble("cup", (double)CUP_WEIGHT);
  scaleMode = preferences.getBool("scaleMode", false);
  grindMode = preferences.getBool("grindMode", true);

  preferences.end();
  
  loadcell.set_scale(calibrationFactor);

  xTaskCreatePinnedToCore(
      updateScale, /* Function to implement the task */
      "Scale",     /* Name of the task */
      10000,       /* Stack size in words */
      NULL,        /* Task input parameter */
      0,           /* Priority of the task */
      &ScaleTask,  /* Task handle. */
      1);          /* Core where the task should run */

  xTaskCreatePinnedToCore(
      scaleStatusLoop, /* Function to implement the task */
      "ScaleStatus", /* Name of the task */
      10000,  /* Stack size in words */
      NULL,  /* Task input parameter */
      0,  /* Priority of the task */
      &ScaleStatusTask,  /* Task handle. */
      1);            /* Core where the task should run */
}
