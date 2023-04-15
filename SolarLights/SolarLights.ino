//#define DEBUG

#include "FastLED.h"

// How many leds are there in your strip?
#define NUM_LEDS 108

// Where is the LED data line connected?
#define LED_DATA 0

// Where is the brightness button connected?
#define BRIGHT_D 2
#define BRIGHT_A 1

// Speed 0..8 (quadratic divisor of milliseconds)
// 0 = so fast that it's definitely not recommended for photosensitive people
// 4 = a nice pace, you could watch it all day
// 7 = so slow, you almost don't see it moving
// 8 and above = animation is stopped
#define SPEED_DIV 3

// the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
  // Initialize the LEDs
  pinMode(LED_DATA, OUTPUT); 
  analogReference(DEFAULT);
  FastLED.addLeds<WS2812B, LED_DATA, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

int pwrlvl = 0;

void loop() {
  // I usually save the current millisecond count per loop.
  uint32_t ms = millis();

  EVERY_N_SECONDS(1) {
    pwrlvl = analogRead(BRIGHT_A);
    #ifdef DEBUG
    Serial.print("power: ");
    Serial.println(pwrlvl);
    #endif
    FastLED.setBrightness(3 + (252 * (pwrlvl < 128)) );
  }

  EVERY_N_MILLISECONDS(10) {

    byte pwr = min(NUM_LEDS, map(pwrlvl, 0, 1024, 0, NUM_LEDS));
    // fill the leds array with a rainbow, starting at a value determined by ms and SPEED_DIV
    fill_rainbow(leds, NUM_LEDS, (ms >> SPEED_DIV) & 0xFF, 255/NUM_LEDS);

    for( int i = 0; i < pwr; i++ ) {
      leds[i] = CRGB(0,0,255);
    }

    if( ((ms >> 9) & 0x01) == 0 ) {
      leds[pwr] = CRGB(255,0,0);
    } else {
      leds[pwr] = CRGB(0,0,255);
    }
    
    //for( int i = 0; i < min(pwrlvl >> 5, NUM_LEDS); i++ ) {
    //  leds[i] = CRGB(255,0,0);
    //}
    
    // push the leds array out to the LED strip
    FastLED.show();
  }

}
