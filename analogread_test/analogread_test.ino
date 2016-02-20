int old = 0;
int delta = 0;

void loop() {
  int cur = analogRead(A1);
  delta = cur - old;
  if( abs(delta) < 2 ) delta = 0;
  Serial.print(cur);
  Serial.print("\t");
  Serial.print(delta);
  Serial.print("\t");
  for( int i = 0; i < cur / 8; i++) {
    Serial.print("-");
  }
  Serial.println();
  delay(10);
  old = cur;
}

void setup() {
  delay(3000);
  pinMode(A1, INPUT);
  Serial.begin(9600);
}
