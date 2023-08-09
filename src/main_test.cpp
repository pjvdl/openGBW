#include <Arduino.h>
#include <MathBuffer.h>
#include <Preferences.h>
#include "display.hpp"
#include "scale.hpp"

HX711 loadcell;
SimpleKalmanFilter kalmanFilter(0.02, 0.02, 0.01);
Preferences preferences;
TaskHandle_t ScaleTask;

double scaleWeight = 0; //current weight
bool wakeDisp = false; //wake up display with rotary click
double setWeight = 0; //desired amount of coffee
double setCupWeight = 0; //cup weight set by user
double offset = 0; //stop x grams prios to set weight
bool scaleMode = false; //use as regular loadcell with timer if true
bool grindMode = true;  //false for impulse to start/stop grinding, true for continuous on while grinding
bool grinderActive = false; //needed for continuous mode
MathBuffer<double, 100> weightHistory;

unsigned long scaleLastUpdatedAt = 0;
bool scaleReady = false;
float calibrationFactor = 4000.0
;
// void tareScale() {
//   Serial.println("Taring loadcell");
//   loadcell.tare(TARE_MEASURES);
//   lastTareAt = millis();
// }

void updateScale( void * parameter) {
  float lastEstimate;


  for (;;) {
    // if (lastTareAt == 0) {
    //   Serial.println("retaring loadcell");
    //   Serial.println("current offset");
    //   Serial.println(offset);
    //   tareScale();
    // }
    if (loadcell.wait_ready_timeout(300)) {
      lastEstimate = kalmanFilter.updateEstimate(loadcell.get_units(5));
      Serial.print("Raw loadcell read value: ");
      Serial.println(loadcell.read());
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


void setupScale() {
  preferences.begin("loadcell", false);
  double scaleFactor = preferences.getDouble("calibration", (double)LOADCELL_SCALE_FACTOR);
  preferences.end();
  
  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  loadcell.set_scale(scaleFactor);

  xTaskCreatePinnedToCore(
      updateScale, /* Function to implement the task */
      "Scale",     /* Name of the task */
      10000,       /* Stack size in words */
      NULL,        /* Task input parameter */
      0,           /* Priority of the task */
      &ScaleTask,  /* Task handle. */
      1);          /* Core where the task should run */
}

void setup() {
  Serial.begin(115200);
  while(!Serial){
    delay(50);
  }
  Serial.println("Serial line initialised");
  printf("Hello world!\n");

//   setupScale();

  Serial.println();
  Serial.println("******************************************************");

  loadcell.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  Serial.println("Before setting up the loadcell:");
  Serial.print("read: \t\t");
  Serial.println(loadcell.read());      // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(loadcell.read_average(20));   // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(loadcell.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight (not set yet)

  Serial.print("get units: \t\t");
  Serial.println(loadcell.get_units(5), 1);  // print the average of 5 readings from the ADC minus tare weight (not set) divided
            // by the SCALE parameter (not set yet)
            
  loadcell.set_scale(calibrationFactor);
  //loadcell.set_scale(-471.497);                      // this value is obtained by calibrating the loadcell with known weights; see the README for details
  loadcell.tare();               // reset the loadcell to 0

  Serial.println("After setting up the loadcell:");

  Serial.print("read: \t\t");
  Serial.println(loadcell.read());                 // print a raw reading from the ADC

  Serial.print("read average: \t\t");
  Serial.println(loadcell.read_average(20));       // print the average of 20 readings from the ADC

  Serial.print("get value: \t\t");
  Serial.println(loadcell.get_value(5));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()

  Serial.print("get units: \t\t");
  Serial.println(loadcell.get_units(5), 1);        // print the average of 5 readings from the ADC minus tare weight, divided
            // by the SCALE parameter set with set_scale

  Serial.println("Readings:");
}

void loop() {
  loadcell.set_scale(calibrationFactor);

  Serial.print("read: \t\t");
  Serial.print(loadcell.read());                 // print a raw reading from the ADC
  Serial.print("read average: \t\t");
  Serial.print(loadcell.read_average(10));       // print the average of 20 readings from the ADC
  Serial.print("get value: \t\t");
  Serial.print(loadcell.get_value(10));   // print the average of 5 readings from the ADC minus the tare weight, set with tare()
  Serial.print("one reading:\t");
  Serial.print(loadcell.get_units(), 1);
  Serial.print("\t| average:\t");
  Serial.println(loadcell.get_units(10), 10);

  if(Serial.available())
  {
    char temp = Serial.read();
    if(temp == '+') {
      calibrationFactor += 20;
      Serial.print("New calibrationFactor of ");
      Serial.println(calibrationFactor);
    } else if (temp == '-') {
      calibrationFactor -= 20;
      Serial.print("New calibrationFactor of ");
      Serial.println(calibrationFactor);
    }
  }

  delay(1000);
}
