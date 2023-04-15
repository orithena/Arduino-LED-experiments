//#define DEBUG

#include "FastLED.h"
#include <Button.h>

// How many leds are there in your strip?
#define NUM_LEDS 7

// Where is the LED data line connected?
#define LED_DATA 1

// Where is the brightness button connected?
#define BRIGHT_PIN 2

// Speed 0..8 (quadratic divisor of milliseconds)
// 0 = so fast that it's definitely not recommended for photosensitive people
// 4 = a nice pace, you could watch it all day
// 7 = so slow, you almost don't see it moving
// 8 and above = animation is stopped
#define SPEED_DIV 6

// button object checking the button pin (it makes sure we get exactly one event per button press) 
Button bright_btn(BRIGHT_PIN);

// state variable for brightness.
byte bright = 4;

// the array of leds
CRGB leds[NUM_LEDS];
CRGB real_leds[NUM_LEDS];

byte mapping[] = { 0, 1, 4, 6, 5, 2, 3 };

void setup() { 
  // Initialize the LEDs
  pinMode(0, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  pinMode(BRIGHT_PIN, INPUT_PULLUP);
  FastLED.addLeds<WS2812B, LED_DATA, GRB>(real_leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(64);
  
  #ifdef DEBUG
  Serial.begin(230400);
  Serial.println("\n\n === Lamp_Simple1Button.ino ===\n\n");
  #endif
}

void loop() {
  digitalWrite(0, LOW);
  // I usually save the current millisecond count per loop.
  uint32_t ms = millis();

  // button handling code
  if( bright_btn.pressed() ) {
    bright = (bright + 1) % 5;
    // our bright state variable counts from 0..4, but setBrightness wants 0..255
    byte real_bright = map(bright, 0, 4, 0, 255);
    // set brightness via FastLED library
    FastLED.setBrightness(real_bright);
    // debugging output via serial link
    #ifdef DEBUG
    Serial.printf("Brightness button pressed. New value: %d (%d)\n", bright, real_bright);
    #endif
  }

  EVERY_N_MILLISECONDS(10) {

    // fill the leds array with a rainbow, starting at a value determined by ms and SPEED_DIV
    fill_rainbow(leds, NUM_LEDS-1, 255 - ((ms >> SPEED_DIV) & 0xFF), 256/6);

    leds[NUM_LEDS-1] = CRGB(255,0,255);
    for( int a = 0; a < NUM_LEDS; a++ ) {
      real_leds[mapping[a]] = leds[a];
    }
  
    // push the leds array out to the LED strip
    FastLED.show();
  }
}
