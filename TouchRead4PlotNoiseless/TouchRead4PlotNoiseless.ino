#define TOUCH_HISTORY_LEN 10
#define TOUCH_HYSTERESIS 2

#include <NoiselessTouchESP32.h>
NoiselessTouchESP32 t5(T5, TOUCH_HISTORY_LEN, TOUCH_HYSTERESIS);
NoiselessTouchESP32 t7(T7, TOUCH_HISTORY_LEN, TOUCH_HYSTERESIS);
NoiselessTouchESP32 t0(T0, TOUCH_HISTORY_LEN, TOUCH_HYSTERESIS);
NoiselessTouchESP32 t1(T1, TOUCH_HISTORY_LEN, TOUCH_HYSTERESIS);
NoiselessTouchESP32 t3(T3, TOUCH_HISTORY_LEN, TOUCH_HYSTERESIS);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(230400);
  pinMode(T5, INPUT);
  pinMode(T0, INPUT);
  pinMode(T1, INPUT);
  pinMode(T7, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.printf("%d,%d,%d,%d,%d\n", t5.read_with_hysteresis(), t7.read_with_hysteresis(), t0.read_with_hysteresis(), t0.value_from_history(), t3.read_with_hysteresis());
  delay(20);
}
