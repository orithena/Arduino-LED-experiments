
#include "FastLED.h"
#include "buttons.h"

#include "ESP8266WiFi.h"

// How many leds in your strip?
#define NUM_LEDS 31
// Where is the LED data line connected?
#define LED_DATA D3
#define BRIGHT_BTN D6
#define SPEED_BTN D5
#define MODE_BTN D7

#define DEBUG

Button bright_btn = Button(BRIGHT_BTN, OneLongShotTimer, HIGH, 500, 2000, 500, SINCE_HOLD); 
Button mode_btn = Button(MODE_BTN, OneShotTimer, HIGH, 200, 2000, SINCE_HOLD); 
//Button speed_btn = Button(SPEED_BTN, OneLongShotTimer, LOW, 200, 2000, SINCE_HOLD); 
byte bright = 2;
byte mode = 0;

enum led_mapping_mode { Z_UP, Z_DOWN, MIRROR2, MIRROR2INV, TWOWAY, TWOWAYINV, CLOCK, COUNTERCLOCK };
enum led_mapping_mode mapmode = MIRROR2;
#define MODE_NUM 8

// Define the array of leds
CRGB real_leds[NUM_LEDS];
CRGB real_leds_old[NUM_LEDS];
CRGB leds[NUM_LEDS];

void setup() { 
  WiFi.forceSleepBegin();

  // Initialize the LEDs
  pinMode(LED_DATA, OUTPUT); 
  
  FastLED.addLeds<WS2812B, LED_DATA, GRB>(real_leds, NUM_LEDS);
  FastLED.setBrightness(bright);
  
  #ifdef DEBUG
  Serial.begin(230400);
  #endif
}

uint32_t lastMotion = 0;

void loop() { 
  uint32_t ms = millis();

  int bright_event = bright_btn.check();
  if( bright_event == ON ) {
    bright = (bright + 1) % 5;
    #ifdef DEBUG
    Serial.printf("Brightness button. New value: %d\n", bright);
    #endif
    lastMotion = ms;
  } else if (bright_event == Hold) {
    mode = (mode + 1) % MODE_NUM;
    Serial.printf("Mode button. New value: %d\n", mode);
    lastMotion = ms;
  }

  if( mode_btn.check() == ON ) {
    lastMotion = ms;
    Serial.println("Got Motion Event");
  }
  
  EVERY_N_MILLISECONDS(10) {
    FastLED.setBrightness((0xFF >> (4-bright)) * (bright>0) * (ms-lastMotion<300000));
  
    switch( mode ) {
      case 0:
      fill_rainbow(leds, NUM_LEDS, (ms >> 5) & 0xFF, 255/NUM_LEDS);
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
    mapleds( led_mapping_mode(mapmode) );
    boolean changed = false;
    for( int a = 0; a < NUM_LEDS; a++ ) {
      if( real_leds[a] != real_leds_old[a] ) {
        changed = true;
        real_leds_old[a] = real_leds[a];
      }
    }
    //if( changed ) {
      FastLED.show();
    //}
  }
  wdt_reset();
}

void mapleds(enum led_mapping_mode mapping) {
  switch(mapping) {
  case Z_UP:
    for( int a = 0; a < NUM_LEDS; a++ ) {
      real_leds[a] = leds[a];
    }
    break;
  case Z_DOWN:
    for( int a = 0; a < NUM_LEDS; a++ ) {
      real_leds[NUM_LEDS - a - 1] = leds[a];
    }
    break;
  case MIRROR2:
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[a*2];
      real_leds[a+(NUM_LEDS/2)] = leds[a*2];
    }
    break;
  case MIRROR2INV:
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[NUM_LEDS - (a*2) - 1];
      real_leds[a+(NUM_LEDS/2)] = leds[NUM_LEDS - (a*2) - 1];
    }
    break;
  case TWOWAY:
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[a*2];
      real_leds[a+(NUM_LEDS/2)] = leds[NUM_LEDS - (a*2) - 1];
    }
    break;
  case TWOWAYINV:
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[NUM_LEDS - (a*2) - 1];
      real_leds[a+(NUM_LEDS/2)] = leds[a*2];
    }
    break;
  case CLOCK:
    for( int a = 0; a < (NUM_LEDS*0.5); a++ ) {
      real_leds[NUM_LEDS-a-1] = leds[a];
    }
    for( int a = (NUM_LEDS/2); a < NUM_LEDS; a++ ) {
      real_leds[a - (NUM_LEDS/2)] = leds[a];
    }
    break;
  case COUNTERCLOCK:
    for( int a = 0; a < (NUM_LEDS*0.5); a++ ) {
      real_leds[(NUM_LEDS/2)+a] = leds[a];
    }
    for( int a = (NUM_LEDS/2); a < NUM_LEDS; a++ ) {
      real_leds[NUM_LEDS-a-1] = leds[a];
    }
    break;
  }
}

inline boolean between(int a, int x, int b) {
  return (x >= a) && (x <= b);
}


#ifdef DEBUG

#endif

