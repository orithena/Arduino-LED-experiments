
#define SENSOR_PIN 2
#define SENSOR_INT 0
#define SENSOR_THRESHOLD 50
#define SENSOR_INCREMENT 2

volatile int sensor_count = 0;
volatile boolean sensor_old = false;
volatile unsigned long int sensor_last_rising = 0;

void sensor_setup() {
  pinMode(SENSOR_PIN, INPUT);
  attachInterrupt(SENSOR_INT, sensor_isr, RISING);
}

void sensor_isr() {
  int sensor = (digitalRead(SENSOR_PIN) == HIGH);
  if( sensor && sensor_count < SENSOR_THRESHOLD) {
    sensor_count += SENSOR_INCREMENT;
    sensor_last_rising = millis();
  }
}

boolean read_sensor() {
  if( sensor_last_rising < millis() - 1000 ) {
    sensor_count = 0;
  }
  for( int a = 0; a < NUM_LEDS; a++ ) {
    if( a == sensor_count ) {
      leds[ a ] = CRGB(0,0,255);
    } else {
      leds[ a ] = CRGB(0,0,0);
    }
  }
  if( sensor_count >= SENSOR_THRESHOLD ) {
    leds[0] = CRGB(255,0,0);
    FastLED.show();
    sensor_count = 0;
    return true;
  } else {
    FastLED.show();
    return false;
  }
}
