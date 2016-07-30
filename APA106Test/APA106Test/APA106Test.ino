#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 16
// Where is the LED data line connected?
#define DATA_PIN 2

// Define the array of leds
CRGB leds[NUM_LEDS];


void setup() { 
  // Initialize the LEDs
  pinMode(13, OUTPUT); 
  pinMode(DATA_PIN, OUTPUT); 
  FastLED.addLeds<APA106, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

int offset = 0;
void loop() { 

  for( int i = 0; i < NUM_LEDS; i++ ) {
  
    leds[i] = CHSV((offset + (i*10)) % 256,255,255);

    FastLED.show();
    delay(1);
  }
  offset = (offset + 4) % 256;
}

