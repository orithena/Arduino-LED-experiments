
#include <FastLED.h>


void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  sensor_setup();
  led_setup();
}

void loop() {
  boolean x = read_sensor();
}

