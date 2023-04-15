
#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 6
// Where is the LED data line connected?
#define LED_DATA 1
// IR sensor enable
#define IR_D 2
// IR sensor analog in (A1 equals P5)
#define IR_A 1

#define SAMPLES 128
#define AVG 64
#define HYST 2

int samples[SAMPLES];
byte ro = 0;
byte rc = 0;
byte w = 0;
long old_grad = 0;

//#define DEBUG

#define MODE_NUM 8

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 

  // Initialize the LEDs
  pinMode(LED_DATA, OUTPUT);
  pinMode(IR_D, INPUT);
  int sample = analogRead(IR_A);
  init_sampler(sample);
  
  FastLED.addLeds<WS2812B, LED_DATA, GRB>(leds, NUM_LEDS).setCorrection(TypicalSMD5050);
  FastLED.setBrightness(255);
}

uint16_t hue = 0;

void loop() { 
  uint32_t ms = millis();
  add_sample(analogRead(IR_A));
  long g = -grad();
  hue += min(max(g, -3), 3);

//  FastLED.setBrightness(191 + max(min(g, 63), -63));
  
  fill_rainbow(leds, NUM_LEDS, ((ms>>5)+hue) & 0xFF, 255/NUM_LEDS);
//  fill_rainbow(leds, NUM_LEDS, ((ms>>5)+hue) & 0xFF, (255/NUM_LEDS)/max(1, abs(g)));
//  fill_rainbow(leds, NUM_LEDS, ((ms>>5)+hue) & 0xFF, (255)/(NUM_LEDS+max(g, 1-NUM_LEDS)));
//  fill_rainbow(leds, NUM_LEDS, ((ms>>5)+hue) & 0xFF, max(g+(255/NUM_LEDS), 1));
  FastLED.show();
  delay(10);
}

void init_sampler(int sample) {
  for( int a = 0; a < SAMPLES; a++) {
    samples[a] = sample;
  }
  ro = 0;
  rc = SAMPLES-AVG;
  w = 0;
  old_grad = 0;
}

void add_sample(int sample) {
  samples[w] = sample;
  w = (w+1) % SAMPLES;
  ro = (ro+1) % SAMPLES;
  rc = (rc+1) % SAMPLES;
}

long avg(int s) {
  long sum = 0;
  for( int a = s; a < s+AVG; a++ ) {
    sum += samples[a % SAMPLES];
  }
  return sum/AVG;
}

long grad() {
  long old = avg(ro);
  long cur = avg(rc);
  int gradient = cur - old;
  if( gradient == 0 || abs(gradient - old_grad) > HYST ) {
    old_grad = gradient;
  }
  return old_grad;
}
