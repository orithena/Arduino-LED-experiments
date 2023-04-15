#define LIGHT_STATUS_LED
//#define DEBUG

#define IR_LED1_PIN D5
#define IR_LED2_PIN D6
#define ANALOG_PIN A0
#define SETUP_BTN D8
#define LED_PIN D0
#define OUT_PIN D7
#define NUM_IR_LEDS 2
#define INITIATION_DURATION 5000
#define MEASURE_HIGH_US 80
#define MEASURE_LOW_MS 5
#define MEASURE_CYCLE_WAIT 5

volatile byte led_state = 0;
uint32_t measures[NUM_IR_LEDS][3];
uint32_t measure[NUM_IR_LEDS];
uint32_t threshold[NUM_IR_LEDS];
uint32_t min_measure[NUM_IR_LEDS];
boolean obstructed = false;
uint32_t setup_start = 0;
boolean initiation_phase = true;

void setup() {
  pinMode(IR_LED1_PIN, OUTPUT);
  pinMode(IR_LED2_PIN, OUTPUT);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(SETUP_BTN, INPUT);
#ifdef LIGHT_STATUS_LED
  pinMode(LED_PIN, OUTPUT);
#endif
  pinMode(OUT_PIN, OUTPUT);

  Serial.begin(115200);

  threshold[0] = threshold[1] = 0xFFFFFF;
  min_measure[0] = min_measure[1] = 0xFFFFFF;
  setup_start = millis();
  initiation_phase = true;
}

void loop() {
    if( millis() < INITIATION_DURATION ) {
      threshold[0] = threshold[1] = 0xFFFFFF;
      min_measure[0] = min_measure[1] = 0xFFFFFF;
      setup_start = millis();
      initiation_phase = true;
    }
    delay(MEASURE_CYCLE_WAIT);
    measure_cycle();
#ifdef DEBUG
    printbars();
#endif
    if( digitalRead(SETUP_BTN) == HIGH ) {
      min_measure[0] = min_measure[1] = 0xFFFFFF;
      setup_start = millis();
      initiation_phase = true;
    } else {
#ifdef LIGHT_STATUS_LED
      digitalWrite(LED_PIN, !obstructed);
#endif
      digitalWrite(OUT_PIN, obstructed);
    }
}

void measure_cycle() {
  for( int cycle = 0; cycle < 3; cycle++) {
    for( int led = 0; led < NUM_IR_LEDS; led++ ) {
      int led_pin = (led == 0 ? IR_LED1_PIN : IR_LED2_PIN);
      digitalWrite(led_pin, HIGH);
      delayMicroseconds(MEASURE_HIGH_US);
      uint32_t high_measure = analogRead(ANALOG_PIN);
      digitalWrite(led_pin, LOW);
      delay(MEASURE_LOW_MS);
      uint32_t low_measure = analogRead(ANALOG_PIN);
      measures[led][cycle] = (high_measure > low_measure ? high_measure - low_measure : 0);
#ifdef DEBUG
      Serial.print(measures[led][cycle]);
      if( measures[led][cycle] < threshold[led] ) {
        Serial.print("!");
      }
      Serial.print("\t");
#endif
    }
#ifdef DEBUG
    Serial.print("|\t");
#endif
  }
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    measure[led] = 0;
    for( int cycle = 0; cycle < 3; cycle++ ) {
      if( measure[led] < measures[led][cycle]) {
        measure[led] = measures[led][cycle];
      }
    }
#ifdef DEBUG
    Serial.print(measure[led]);
    Serial.print(" > ");
    Serial.print(threshold[led]);
    Serial.print("\t");
#endif
  }
  if( initiation_phase ) {
    for( int led = 0; led < NUM_IR_LEDS; led++ ) {
      if( min_measure[led] > measure[led] ) {
        min_measure[led] = measure[led];
        threshold[led] = 0.95 * min_measure[led];
      }
#ifdef DEBUG
      Serial.print(min_measure[led]);
      Serial.print("\t");
      Serial.print(threshold[led]);
      Serial.print("\t");
#endif
    }
    if( millis() > (setup_start + INITIATION_DURATION) ) initiation_phase = false;
  }
  obstructed = false;
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    if( measure[led] < threshold[led] ) {
      obstructed = true;
    }
  }
#ifdef DEBUG
  Serial.println(obstructed ? "!!!" : "...");
#endif
}

#ifdef DEBUG
void printbars() {
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    char c1 = '#'+led;
    char c2 = '-';
    printbar(measure[led]>>2, threshold[led]>>2, c1, c2); 
  }
}

void printbar(int n, int t, char c1, char c2) {
  for( int i = 0; i < n; i++ ) {
    if( i <= t ) {
      Serial.print(c1);
    } else {
      Serial.print(c2);
    }
  }
  Serial.println();
}
#endif

