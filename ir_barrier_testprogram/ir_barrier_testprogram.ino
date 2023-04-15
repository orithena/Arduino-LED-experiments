
//#define LIGHT_STATUS_LED
#define DEBUG

#define NUM_IR_LEDS 2
#define IR_LED1_PIN D5
#define IR_LED2_PIN D6
#define ANALOG_PIN A0
#define SETUP_BTN D8
#define LED_PIN D4
#define OUT_PIN D7

#define MEASURE_CYCLES 3
#define MEASURE_HIGH_US 60
#define MEASURE_LOW_US 80

#define THRESHOLD_FACTOR 0.875

#define FOLLOW_SPEED 0.125
#define FOLLOW_SPEED_UP 0.25
#define FOLLOW_SPEED_INIT2 1.0
#define FOLLOW_SPEED_INIT5 0.5
#define FOLLOW_SPEED_INIT20 0.25

#define OUTPUT_HIGH_US 10000

#ifdef DEBUG
#define SLEEP_FOR_S 2
#else
#define SLEEP_FOR_S 60
#endif
#define INIT_SLEEP_FOR_S 1

#define STATE_OFFSET 16
#define MAGIC_NUMBER 0xDEADBEEF

typedef struct State {
  uint32_t magic_number;
  uint32_t cycle_counter;
  uint32_t measure[NUM_IR_LEDS];
  float mean_measure[NUM_IR_LEDS];
  boolean initiation_phase;
} State;

uint32_t measures[NUM_IR_LEDS][3];
boolean obstructed = false;

void setup() {
#ifdef DEBUG
  Serial.begin(230400);
  Serial.println("Hello.\t");
#endif
  set_pin_modes();

  State p = read_state();

  measure_cycle(&p);

#ifdef DEBUG
    printbars(&p);
#endif
    if( digitalRead(SETUP_BTN) == HIGH ) {
      p.initiation_phase = true;
    } else {
#ifdef LIGHT_STATUS_LED
      digitalWrite(LED_PIN, obstructed);
#endif
      if( obstructed ) {
        pinMode(OUT_PIN, OUTPUT);
        digitalWrite(OUT_PIN, LOW);
        delayMicroseconds(OUTPUT_HIGH_US);
        pinMode(OUT_PIN, INPUT);
      }
    }
  save_state(p);
  ESP.deepSleep( (p.initiation_phase ? INIT_SLEEP_FOR_S : SLEEP_FOR_S) * 1000000L);
}

void loop() {
}

void set_pin_modes() {
  pinMode(IR_LED1_PIN, OUTPUT);
  pinMode(IR_LED2_PIN, OUTPUT);
  pinMode(ANALOG_PIN, INPUT);
  pinMode(SETUP_BTN, INPUT);
  pinMode(OUT_PIN, INPUT);
}

void measure_cycle(State *p) {
#ifdef DEBUG
  Serial.print(p->cycle_counter);
  Serial.print("\t");
#endif
  for( int cycle = 0; cycle < MEASURE_CYCLES; cycle++) {
    for( int led = 0; led < NUM_IR_LEDS; led++ ) {
      int led_pin = (led == 0 ? IR_LED1_PIN : IR_LED2_PIN);
      digitalWrite(led_pin, HIGH);
      delayMicroseconds(MEASURE_HIGH_US);
      int32_t high_measure = analogRead(ANALOG_PIN);
      digitalWrite(led_pin, LOW);
      delayMicroseconds(MEASURE_LOW_US);
      int32_t low_measure = analogRead(ANALOG_PIN);
      measures[led][cycle] = max( high_measure - low_measure, 0 );
#ifdef DEBUG
      Serial.print(high_measure);
      Serial.print("\t");
      Serial.print(low_measure);
      Serial.print("\t");
      Serial.print(measures[led][cycle]);
      Serial.print("\t");
#endif
    }
  }
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    p->measure[led] = 0;
    for( int cycle = 0; cycle < MEASURE_CYCLES; cycle++ ) {
      if( p->measure[led] < measures[led][cycle]) {
        p->measure[led] = measures[led][cycle];
      }
    }
  }
  
  obstructed = false;
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    if( p->measure[led] < (p->mean_measure[led] * THRESHOLD_FACTOR) ) {
      #ifdef DEBUG
        Serial.print("500");
      #endif
      obstructed = true;
    }
    #ifdef DEBUG
    else {
        Serial.print("0");
    }
    Serial.print("\t");
    #endif

  }
      
  float follow_speed = FOLLOW_SPEED;
  if( p->initiation_phase ) {
    if( p->cycle_counter < 2 ) {
      follow_speed = FOLLOW_SPEED_INIT2;
    } else if( p->cycle_counter < 5 ) {
      follow_speed = FOLLOW_SPEED_INIT5;
    } else if( p->cycle_counter < 20 ) {
      follow_speed = FOLLOW_SPEED_INIT20;
    } else {
      p->initiation_phase = false;
    }
  }
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    float delta = p->measure[led] - p->mean_measure[led];
    if( delta > 10.0 ) {
      p->mean_measure[led] += (delta * max(follow_speed, FOLLOW_SPEED_UP));
    } else {
      p->mean_measure[led] += (delta * follow_speed);
    }
#ifdef DEBUG
      Serial.print(p->measure[led]);
      Serial.print("\t");
      Serial.print(p->mean_measure[led]);
      Serial.print("\t");
      Serial.print(p->mean_measure[led] * THRESHOLD_FACTOR);
      Serial.print("\t");
#endif
  }
#ifdef DEBUG
  Serial.println();
#endif

  p->cycle_counter++;
}

struct State read_state() {
  State p;
  ESP.rtcUserMemoryRead(STATE_OFFSET, (uint32_t*)(&p), sizeof(State));
  if( p.magic_number != MAGIC_NUMBER ) {
    // init
    p.initiation_phase = true;
    p.magic_number = MAGIC_NUMBER;
    p.cycle_counter = 0;
    for( int led = 0; led < NUM_IR_LEDS; led++ ) {
      p.mean_measure[led] = 0.0;
    }
  }
  return p;
}

void save_state(State p) {
  ESP.rtcUserMemoryWrite(STATE_OFFSET, (uint32_t*)(&p), sizeof(State));
}

#ifdef DEBUG
void printbars(State *p) {
  for( int led = 0; led < NUM_IR_LEDS; led++ ) {
    char c1 = '#'+led;
    char c2 = '-';
    printbar(p->measure[led]>>2, (p->mean_measure[led]*THRESHOLD_FACTOR)/4, c1, c2); 
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


