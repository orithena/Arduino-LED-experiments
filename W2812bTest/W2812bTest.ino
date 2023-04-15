#include "FastLED.h"
#include "buttons.h"

// How many leds in your strip?
#define NUM_LEDS 12
// Where is the LED data line connected?
#define DATA_PIN D3
#define BRIGHT_BTN D5
#define MODE_BTN D6

Button bright_btn, mode_btn;
byte bright = 0;
byte mode = 0;

// Define the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
  // Initialize the LEDs
  pinMode(DATA_PIN, OUTPUT); 
  bright_btn.assign(BRIGHT_BTN); 
  mode_btn.assign(MODE_BTN); 
  bright_btn.setMode(OneShotTimer);
  mode_btn.setMode(OneShotTimer);
  FastLED.addLeds<WS2812B, DATA_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

#define MODE_NUM 9

byte offset = 0;
void loop() { 
  if( bright_btn.check() == ON ) {
    bright = (bright + 1) % 5;
  }
  if( mode_btn.check() == ON ) {
    mode = (mode + 1) % MODE_NUM;
  }

  //FastLED.setBrightness(min(255, bright*64));
  switch( mode ) {
    case 0:
    fill_rainbow(leds, NUM_LEDS, offset, 7);
    break;
    case 1:
    fill_rainbow(leds, NUM_LEDS, 0, 255/NUM_LEDS);
    break;
    case 2:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(4,4,4), CRGB(255,255,255));
    break;
    case 3:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(4,0,0), CRGB(255,0,0));
    break;
    case 4:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(4,4,0), CRGB(255,255,0));
    break;
    case 5:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(0,4,0), CRGB(0,255,0));
    break;
    case 6:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(0,4,4), CRGB(0,255,255));
    break;
    case 7:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(0,0,4), CRGB(0,0,255));
    break;
    case 8:
    fill_gradient_RGB(leds, NUM_LEDS, CRGB(4,0,4), CRGB(255,0,255));
    break;
  }
  FastLED.show();
  delay(10);
  offset++;
}
