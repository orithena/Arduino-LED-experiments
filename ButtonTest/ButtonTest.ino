#include "FastLED.h"
#include "buttons.h"
Button btn = Button(D6, OneLongShotTimer, LOW, 200, 2000, SINCE_HOLD);

void setup() {
  Serial.begin(230400);
}

void loop() {
  EVERY_N_MILLISECONDS(100) {
    int b = btn.check();
    Serial.printf("%10d: Button state = %d %s\n", millis(), b,
      b == ON ? "ON" :
      b == OFF ? "OFF" :
      b == Pressed ? "Pressed" :
      b == Released ? "Released" :
      b == Hold ? "Hold" : 
      b > Hold ? "Holding" : "UNKNOWN"
    );
    if( b == Released ) {
      Serial.printf("Released after %d ms\n", btn.getLastHoldTimespan());
    }
  }
}

