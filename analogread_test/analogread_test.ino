int old = 0;
int delta = 0;

void loop() {
  int cur = analogRead(A0);
  delta = cur - old;
  if( abs(delta) < 2 ) delta = 0;
  if( delta != 0 ) {
    Serial.print(micros());
    Serial.print("\t");
    Serial.print(cur);
    Serial.print("\t");
    Serial.print(delta);
    Serial.print("\t");
    for( int i = 0; i < cur / 8; i++) {
      Serial.print("-");
    }
    Serial.println();
  }
  old = cur;
}

void setup() {
  delay(3000);
  pinMode(A0, INPUT);
  Serial.begin(230400);
}
