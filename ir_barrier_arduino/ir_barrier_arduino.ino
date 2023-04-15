#define IR_LED_PIN 10
#define ANALOG_PIN A7
#define SETUP_BTN 8
#define LED_PIN 13
#define COOLDOWN 1000

uint32_t threshold = 0;
uint32_t min_measure = 0;
uint32_t setup_start = 0;
uint32_t last_threshold = 0;
boolean initiation_phase = true;

void setup() {
  pinMode(IR_LED_PIN, OUTPUT);
  pinMode(ANALOG_PIN, INPUT_PULLUP);
  pinMode(SETUP_BTN, INPUT_PULLUP);
  pinMode(LED_PIN, OUTPUT);
//  Serial.begin(230400);
  threshold = 0xFFFFFF;
  min_measure = 0xFFFFFF;
  initiation_phase = true;
  last_threshold = 0;
}

void loop() {
    delay(10);
    digitalWrite(IR_LED_PIN, HIGH);
    delay(1);
    uint32_t high_measure = analogRead(ANALOG_PIN);
    digitalWrite(IR_LED_PIN, LOW);
    delay(10);
    uint32_t low_measure = analogRead(ANALOG_PIN);
    uint32_t measure = (high_measure - low_measure);
    if( initiation_phase ) {
      if( min_measure > measure ) {
        min_measure = measure;
        threshold = 0.8 * min_measure;
      }
      if( millis() > setup_start + 2000 ) initiation_phase = false;
    }
//    Serial.print(measure);
//    printbar(measure, threshold);
    if( digitalRead(SETUP_BTN) == LOW ) {
      digitalWrite( LED_PIN, ((millis()>>6) % (256/measure)) < 10 );
      min_measure = 0xFFFFFF;
      setup_start = millis();
      initiation_phase = true;
    } else {
      if( measure < threshold ) {
        last_threshold = millis();
      }
      digitalWrite(LED_PIN, (millis() - last_threshold) < COOLDOWN);
    }
}

void printbar(int n, int t) {
  for( int i = 0; i < n; i++ ) {
    if( i <= t ) {
      Serial.print("#");
    } else {
      Serial.print("$");
    }
  }
  Serial.println();
}

