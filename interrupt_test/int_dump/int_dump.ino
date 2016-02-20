int pin = 2;
volatile int state = 0;
volatile int micro = 0;


void setup() {
    Serial.begin(115200);
    pinMode(pin, INPUT);
    Serial.print(digitalRead(pin));
    Serial.print(" ");
    Serial.println(micros());
    attachInterrupt(0, dump, CHANGE);
}

void loop() {
    Serial.print(state);
    Serial.print(" ");
    Serial.println(micro); 
}

void dump() {
  state = digitalRead(pin);
  micro = micros();
}
