#define PIN D6
volatile int state = 0;
int oldstate = 0;
volatile uint32_t micro = 0;
volatile uint32_t ms = 0;


void setup() {
    delay(2000);
    Serial.begin(230400);
    pinMode(PIN, INPUT_PULLUP);
    Serial.println(" ");
    Serial.print(digitalRead(PIN));
    Serial.print(" ");
    Serial.println(micros());
    attachInterrupt(PIN, falling, FALLING);
//    attachInterrupt(PIN, rising, RISING);
}

void loop() {
  if( state > 0 ) {
    Serial.print("\t");
    Serial.print(state == 1 ? "FALL" : "RISE");
    Serial.print("\t");
    Serial.print(micro);
    Serial.print("\t");
    Serial.println(ms);
    oldstate = state;
  }
  state = 0;
}

ICACHE_RAM_ATTR void falling() {
  state = 1;
  micro = micros();
  ms = millis();
}

ICACHE_RAM_ATTR void rising() {
  state = 2;
  micro = micros();
}
