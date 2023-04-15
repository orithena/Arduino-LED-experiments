//#define DEBUG

#include "FastLED.h"
#include <minibutton.h>

// How many leds are there in your strip?
#define NUM_LEDS 28
#define VLEDS 14

// Where is the LED data line connected?
#define LED_DATA 0

// Where is the brightness button connected?
#define BRIGHT_PIN 2

#define KEEPALIVE_PIN 4

// Speed 0..8 (quadratic divisor of milliseconds)
// 0 = so fast that it's definitely not recommended for photosensitive people
// 4 = a nice pace, you could watch it all day
// 7 = so slow, you almost don't see it moving
// 8 and above = animation is stopped
#define SPEED_DIV 5

// button object checking the button pin (it makes sure we get exactly one event per button press) 
Button bright_btn(BRIGHT_PIN, OneLongShotTimer, HIGH, 500, 5500, 100, SINCE_HOLD);

#define MODE_COUNT 8
#define MIRROR_COUNT 4

// state variable for brightness.
int bright = 3;
byte mode = 0;
byte mirror = 0;
uint32_t last_on = 0;

uint32_t keepalive_last = 0;
const uint32_t keepalive_high = 5120;
const uint32_t keepalive_low = 128;
int keepalive_status = LOW;

// the array of leds
CRGB leds[NUM_LEDS];

void setup() { 
  pinMode(KEEPALIVE_PIN, OUTPUT);
  digitalWrite(KEEPALIVE_PIN, HIGH); 
  // make sure the Wifi chip is off, we don't need it.
  // Initialize the LEDs
  pinMode(LED_DATA, OUTPUT); 
  FastLED.addLeds<WS2812B, LED_DATA, GRB>(leds, NUM_LEDS);
  
  #ifdef DEBUG
  Serial.begin(115200);
  #endif

  pinMode(2, INPUT); //LED on Model A   
}

void loop() {
  // I usually save the current millisecond count per loop.
  uint32_t ms = millis();

    // button handling code
  int b = bright_btn.check();
  if( last_on != 0 && ms - last_on > 200 ) {
    mode = (mode + 1) % MODE_COUNT;
    last_on = 0;
  } else if( b == ON && ms-last_on < 200) {
    mirror = (mirror + 1) % MIRROR_COUNT;
    last_on = 0;
  } else if( b == ON ) {
    last_on = ms;
  } else if( b > Hold ) {
    bright = min(5000, max(500, b) - 500);
    byte real_bright = 0xFF >> (map(bright, 0, 5000, 8, 0));
    // set brightness via FastLED library
    FastLED.setBrightness(real_bright);
    // debugging output via serial link
    #ifdef DEBUG
    Serial.println(real_bright);
    #endif
  }

  EVERY_N_MILLISECONDS(10) {

    for( int i = 0; i < NUM_LEDS; i++ ) {
      leds[i] = CRGB(0,0,0);
    }
    // fill the leds array with a rainbow, starting at a value determined by ms and SPEED_DIV
    //fill_rainbow(leds, NUM_LEDS, (ms >> SPEED_DIV) & 0xFF, 255/NUM_LEDS);
    int firstled = (mirror > 1) ? 7 : 0; 
    for( int i = firstled; i < VLEDS; i++ ) {
      switch( mode ) {
      case 0:
        leds[i] = CHSV(((VLEDS-i)<<4) + (ms>>5), 255,255);
        break;
      case 1:
        leds[i] = CRGB(255,0,0);
        break;
      case 2:
        leds[i] = CRGB(255,255,0);
        break;
      case 3:
        leds[i] = CRGB(0,255,0);
        break;
      case 4:
        leds[i] = CRGB(0,255,255);
        break;
      case 5:
        leds[i] = CRGB(0,0,255);
        break;
      case 6:
        leds[i] = CRGB(255,0,255);
        break;
      case 7:
        leds[i] = CRGB(255,255,255);
        break;
      }
    }
    
    for( int i = firstled; i < VLEDS; i++) {
      if( mirror % 2 == 0 ) {
        leds[NUM_LEDS-i-1] = leds[i];
      }
    }
    // push the leds array out to the LED strip
    FastLED.show();
  }

  if( keepalive_status == HIGH && ms - keepalive_last > keepalive_high ) {
    digitalWrite(KEEPALIVE_PIN, LOW);
    keepalive_status = LOW;
    keepalive_last = ms;
  }
  if( keepalive_status == LOW && ms - keepalive_last > keepalive_low ) {
    digitalWrite(KEEPALIVE_PIN, HIGH);
    keepalive_status = HIGH;
    keepalive_last = ms;
  }
}
