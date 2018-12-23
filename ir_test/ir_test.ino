#define IR_LED_PIN D5
#define IR_REC_PIN D6
#define ANALOG_PIN A0
#define LED_PIN D4

volatile byte led_state = 0;

void setup() {
  // put your setup code here, to run once:
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(IR_REC_PIN, INPUT);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
//  attachInterrupt(IR_REC_PIN, irq, CHANGE);
  Serial.begin(230400);
  analogWriteFreq(38000);
}

void loop() {
//  for( int f = 5000; f < 50000; f += 1000 ) {
//    Serial.println(f);
//    analogWriteFreq(f);

    digitalWrite(IR_LED_PIN, HIGH);
    for( int i = 0; i < 1000; i++ ) {
      //digitalWrite(LED_PIN, analogRead(IR_REC_PIN));
      delay(1);
      printAnalog();
    }
    digitalWrite(IR_LED_PIN, LOW);
    for( int i = 0; i < 1000; i++ ) {
      //digitalWrite(LED_PIN, analogRead(IR_REC_PIN));
      delay(1);
      printAnalog();
    }
//  }
}

void printAnalog() {
  int in = analogRead(A0);
  for( int i = 0; i < in; i++ ) {
    Serial.print("#");
  }
  Serial.println();
}

//void irq() {
//  digitalWrite(LED_PIN, digitalRead(IR_REC_PIN));
//}

