#define PIN D3
volatile int state = 0;
int oldstate = 0;
volatile uint32_t micro = 0;


void setup() {
    delay(2000);
    Serial.begin(230400);
    pinMode(PIN, INPUT);
    Serial.println(" ");
    Serial.print(digitalRead(PIN));
    Serial.print(" ");
    Serial.println(micros());
    attachInterrupt(PIN, dump, FALLING);
}

void loop() {
  if( state > 0 ) {
    Serial.print(digitalRead(PIN));
    Serial.print("\t");
    Serial.print(state);
    Serial.print("\t");
    Serial.println(micro);
    oldstate = state;
  }
  state = 0;
}

void dump() {
//  state = digitalRead(PIN);
  state = 1;
  micro = micros();
}
