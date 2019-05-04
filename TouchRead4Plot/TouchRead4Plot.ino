void setup() {
  // put your setup code here, to run once:
  Serial.begin(230400);
  pinMode(T9, INPUT);
  pinMode(T5, INPUT);
  pinMode(T0, INPUT);
  pinMode(T1, INPUT);
}

void loop() {
  // put your main code here, to run repeatedly:
  Serial.printf("%d,%d,%d,%d\n", touchRead(T9), touchRead(T5), touchRead(T0), touchRead(T1));
  delay(20);
}
