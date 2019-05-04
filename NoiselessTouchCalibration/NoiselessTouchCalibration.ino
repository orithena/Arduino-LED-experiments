
/*  NoiselessTouchCalibration.ino
 *  
 *  Experiment around with HISTORY_LEN and HYSTERESIS until
 *  you find the right values for your application.
 *  
 *  The raw mean value is varying to a small degree. The hysteresis 
 *  value should be at least half the difference between the "normal"
 *  raw mean value and the lowest raw mean value observed while not
 *  touching anything (YOUR definition of "touching" applies here, 
 *  i.e. wave around near the electrode, stay away, go near, etc to
 *  observe the highest and lowest raw mean value while not touching).
 *  
 *  The hysteresis value should be at most half the difference
 *  between the raw mean value of a definitive non-touch and
 *  the raw mean value of a definitive touch. (If that difference
 *  is below 2, the library cannot help you. You need to change
 *  your physical setup to get clearer readings.)
 */

#define TOUCH_PIN T6
#define TOUCH_HISTORY_LEN 6
#define TOUCH_HYSTERESIS 3

#include <NoiselessTouchESP32.h>
NoiselessTouchESP32 touchsensor(TOUCH_PIN, TOUCH_HISTORY_LEN, TOUCH_HYSTERESIS);

void setup() {
  Serial.begin(230400);
  Serial.printf("\n\n === NoiselessTouchESP32 test sketch ===\n\n");
}

int highest = 0;
int lowest = 255;

void loop() {
  // !!! Uncomment the event method you want to test, comment the others:
  //int touch = touchsensor.touching();
  //int touch = touchsensor.touched();
  int touch = touchsensor.changed();

  int raw_mean_value = touchsensor.value_from_history();
  highest = _max(highest, raw_mean_value);
  lowest = _min(lowest, raw_mean_value);

  Serial.printf(
    "Event: %2d\tvalue with hysteresis: %3d\traw mean value: %3d [lowest:%3d, highest:%3d]\n",
    touch,
    touchsensor.last_value(),
    raw_mean_value,
    lowest,
    highest
  );

  // Increase delay to something like 500 if you want to take your time to watch the values.
  delay(50);
}

