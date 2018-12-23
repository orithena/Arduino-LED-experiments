

void setup() {
  // put your setup code here, to run once:
  pinMode(D7, INPUT);
  pinMode(D4, OUTPUT);
  
  Serial.begin(230400);
  Serial.println("\n\nir_test_digital\n");
}

boolean old = false;
uint32_t count = 0;

void loop() {
  boolean cur = digitalRead(D7);
  if( old != cur ) {
    count++;
    Serial.printf("%8d: State changed to: %d\n", count, cur);
    old = cur;
    digitalWrite(D4, !cur);
  }
}
