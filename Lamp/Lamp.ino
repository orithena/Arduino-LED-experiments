#include <ESP8266WiFi.h>
#include "FastLED.h"
#include "buttons.h"

#include <EEPROM.h>


// How many leds in your strip?
#define NUM_LEDS 16
// Where is the LED data line connected?
#define LED_DATA D4

// Where are the buttons connected?
#define BRIGHT_BTN D6
#define SPEED_BTN D5
#define MODE_BTN D7

// Minimum Speed number (actually, milliseconds are divided by 2^speed, so low speed numbers equal fast animation)
#define MIN_SPEED 1
// How many speed steps?
#define SPEED_STEPS 8

#define MAX_SLEEP_HOURS (NUM_LEDS/2)
#define MAX_SLEEP_MS (MAX_SLEEP_HOURS * 60 * 60 * 1000)
#define MAX_SLEEP_PUSH_TIME (MAX_SLEEP_HOURS*400)

// Color is increased by this value
#define COLOR_INC (255/15)

// Comment this in if you want to debug via Serial console.
#define DEBUG

// Global Variables for the buttons
Button bright_btn = Button(BRIGHT_BTN, OneLongShotTimer, LOW, 400, 400 + MAX_SLEEP_PUSH_TIME, SINCE_HOLD);
Button mode_btn = Button(MODE_BTN, OneShot, LOW, 400, 2400, SINCE_HOLD);
Button speed_btn = Button(SPEED_BTN, OneShot, LOW, 400, 2400, SINCE_HOLD);

// Where is the state saved? 
#define STATE_ADDRESS 16
// What is our token that confirms that we read a valid state?
#define MAGIC_NUMBER 0xCA7B0A75

bool power_state = true;

// Define the contents of our internal state, read from and written to EEPROM.
typedef struct State {
  uint32_t magic_number;
  byte bright;
  byte mode;
  byte spd;
  byte col;
} State;
// This is the variable for our state.
struct State state;

// How long (in millisecconds) shall we wait after the last user interaction before we save values to EEPROM?
// (EEPROM decays with each write, so we want to make sure not to save too often!)
#define SAVE_GRACE_MS 10000
// Global variable to remember the last user interaction time.
uint32_t last_user_interaction = 0;

// How many modes are programmed in total?
#define MODE_NUM 9
// What is the smallest static mode number? (small mode number => animated mode, big mode number => static mode)
#define STATIC_MODE_MIN 5

// An enumeration of the possible led mappings
enum led_mapping_mode { Z_DOWN, Z_UP, MIRROR2, MIRROR2INV, TWOWAY, TWOWAYINV, MIRROR4INV, MIRROR4, CLOCK, COUNTERCLOCK };

// Define the led arrays
// This is the led output array
CRGB real_leds[NUM_LEDS];
// This is a copy of the led output array, so we can check whether something has changed
CRGB real_leds_old[NUM_LEDS];
// This is our "working copy" of the led array; it's copied to real_leds by mapleds(led_mapping_mode)
CRGB leds[NUM_LEDS];

// lets save whether something has changed so we know whether to update the LEDs.
boolean something_happened = false;

uint32_t sleep_start = 0;
uint32_t sleep_duration = 0;

uint32_t display_mode_start = 0;
enum DisplayMode { EFFECT, SLEEPING, SETSLEEP, ACKMODE, ACKSPEED, ACKCOLOR, ACKBRIGHT };
DisplayMode display_mode = EFFECT;

/*
 * This routine is run once at powerup.
 */
void setup() {
  #ifdef DEBUG
  // Initialize Serial console.
  Serial.begin(230400);
  #endif
  EEPROM.begin(32);

  // read fom EEPROM
  state = read_state();
  // if powered up, we want to see something, so we increase brightness to at least 1
  if( state.bright == 0 ) {
    state.bright = 1;
  }
  
  // initialize LED data output pin.
  pinMode(LED_DATA, OUTPUT); 

  // Initialize LED library
  FastLED.addLeds<WS2812B, LED_DATA, GRB>(real_leds, NUM_LEDS).setCorrection( TypicalSMD5050 );
  // set overall brightness of LEDs
  FastLED.setBrightness((0xFF >> (4-state.bright)) * (state.bright>0));
  // Show something for starters.
  FastLED.show();

  // yes, something happened. This device has just been switched on.
  something_happened = true;
}

/*
 * This routine loops forever
 */
void loop() { 
  EVERY_N_MILLISECONDS(10) {
    // read Button events. This may change state.
    something_happened |= interpret_Buttons(&state);

    // let's save the current time, we'll need it often
    uint32_t ms = millis();
    uint32_t display_ms = ms - display_mode_start; // time since last display mode change
  
    // Now use the state to show something :)

    switch( display_mode ) {
      case EFFECT:
        // Calculate the LEDs from state
        switch( state.mode ) {
          case 0:
          case 1:
          case 2:
          case 3:
          case 4:   // modes 0-4 are just an animated rainbow with different mappings
            fill_rainbow(leds, NUM_LEDS, (ms >> (state.spd+MIN_SPEED)) & 0xFF, 255/NUM_LEDS);
            mapleds( led_mapping_mode(state.mode*2) );
            break;
          case 5:   // mode 5 is a static color gradient, mapped using MIRROR4 (2-axis mirroring)
            fill_gradient_RGB(leds, NUM_LEDS, CHSV(state.col,255,128), CHSV(state.col,255,255));
            mapleds( MIRROR4 );
            break;
          case 6:   // mode 6 is a static color gradient, mapped using TWOWAY
            fill_gradient_RGB(leds, NUM_LEDS, CHSV(state.col,255,32), CHSV(state.col,255,255));
            mapleds( TWOWAY );
            break;
          case 7:
          case 8:   // mode 7 and 8 is a static rainbow starting at state.col, mapped using Z_UP(7) or COUNTERCLOCK(8)
            fill_rainbow(leds, NUM_LEDS, state.col, 256/NUM_LEDS);
            mapleds( state.mode == 7 ? Z_UP : TWOWAYINV );
            break;
        }
        break;
      case SLEEPING:
        if( display_ms < 2000 ) {
          fill_gradient_RGB(real_leds, NUM_LEDS-map(display_ms, 0, 2000, 0, NUM_LEDS-1), CRGB(255,0,0), CRGB(0,0,0));
        } else if(ms - sleep_start > sleep_duration) {
          set_display_mode(EFFECT);
        } else {
          fill_solid(real_leds, NUM_LEDS, CRGB(0,0,0));
        }
        break;
      case SETSLEEP:
        if( display_ms < 500 ) {
          for( int a = 0; a < NUM_LEDS; a++ ) {
            real_leds[a] = 
              (a%2==0) ? CRGB(0,0,0) : 
              map(sleep_duration, 0, MAX_SLEEP_MS, 0, MAX_SLEEP_HOURS)*2 >= a ? CRGB(255, 0, 128) : 
              CRGB( 32, 0, 0);
          }
        } else {
          set_display_mode(SLEEPING);
        }
        break;
      case ACKMODE:
        if( display_ms < 1000 ) {
          for( int a = map(state.mode, 0, MODE_NUM, 0, NUM_LEDS-1); a <= map(state.mode+1, 0, MODE_NUM, 0, NUM_LEDS-1); a++ ) {
            real_leds[a] = CRGB(255,255,255);
          }
        } else {
          set_display_mode(EFFECT);
        }
        break;
      case ACKSPEED:
        if( display_ms <= 600 ) {
          for( int a = 0; a <= map(state.spd, SPEED_STEPS, 0, 0, map(display_ms, 0, 600, 0, NUM_LEDS-1)); a++ ) {
            real_leds[a] = CRGB(255,255,0);
          }
        } else {
          set_display_mode(EFFECT);
        }
        break;
      case ACKCOLOR:
        if( display_ms < 300 ) {
          fill_solid(real_leds, NUM_LEDS, CHSV(state.col, 255, 255));
        } else {
          set_display_mode(EFFECT);
        }
        break;
      default:
        set_display_mode(EFFECT);
        break;
    }
    // check if the output array has changed at all
    for( int a = 0; a < NUM_LEDS; a++ ) {
      if( real_leds[a] != real_leds_old[a] ) {
        something_happened = true;
        real_leds_old[a] = real_leds[a];
      }
    }
  
    // Set brightness. (Shows up only after FastLED.show())
    FastLED.setBrightness((0xFF >> (4-state.bright)) * (state.bright>0) * power_state);
  
    // if the led array has changed, we actually update the LEDs.
    if( something_happened ) {
      FastLED.show();
      // reset the event marker
      something_happened = false;
    }
  
    // save state to EEPROM (the routine checks whether its necessary)
    save_state(state);
  }
}

/*
 * Check whether one of the three hardware buttons has been pressed
 * and change state accordinly.
 */
boolean interpret_Buttons(State* state) {
  boolean something_was_pressed = false;

  int bright = bright_btn.check();
  if( bright == ON ) {
    if( display_mode != EFFECT ) {
      #ifdef DEBUG
      Serial.print("Got brightness button; back to EFFECT display mode  \t");
      print_state(state);
      #endif
      set_display_mode(EFFECT);
    } else {
      state->bright = (state->bright + 1) % 5;
      last_user_interaction = millis();
      #ifdef DEBUG
      Serial.print("Got brightness button. \t");
      print_state(state);
      #endif
      set_display_mode(ACKBRIGHT);
    }
    something_was_pressed = true;
  } else if ( bright >= Hold ) {
    sleep_start = millis();
    sleep_duration = map(bright, 0, MAX_SLEEP_PUSH_TIME, 0, MAX_SLEEP_MS);
    #ifdef DEBUG
    Serial.printf("Got brightness button held for %d, sleep_start=%d, sleep_duration=%d\n", bright, sleep_start, sleep_duration);
    print_state(state);
    #endif
    set_display_mode(SETSLEEP);
    something_was_pressed = true;
  }

  if( mode_btn.check() == ON ) {
    state->mode = (state->mode + 1) % MODE_NUM;
    last_user_interaction = millis();
    something_was_pressed = true;
    #ifdef DEBUG
    Serial.print("Got mode button.       \t");
    print_state(state);
    #endif
    set_display_mode(ACKMODE);
  }

  if( speed_btn.check() == ON ) {
    #ifdef DEBUG
    Serial.print("Got speed/color button.\t");
    #endif
    if( state->mode < STATIC_MODE_MIN ) {
      set_display_mode(ACKSPEED);
      // in animated modes, this button changes speed.
      state->spd = (state->spd + 1) % SPEED_STEPS;
    } else {
      set_display_mode(ACKCOLOR);
      // in static modes, this button changes color.
      // this formula makes sure that the colors are set to a multiple of COLOR_INC while increasing the hue by color_inc
      state->col = (byte)((state->col - (state->col % COLOR_INC) + COLOR_INC) % (256 - (256 % COLOR_INC)));
    }
    last_user_interaction = millis();
    something_was_pressed = true;
    #ifdef DEBUG
    print_state(state);
    #endif
  }
  return something_was_pressed;
}

void set_display_mode(DisplayMode modus) {
  display_mode = modus;
  display_mode_start = millis();
  something_happened = true;
  #ifdef DEBUG
  Serial.printf("Display mode is now %d, starting at %d\n", display_mode, display_mode_start);
  print_state(&state);
  #endif
}

void cycle_speed( State* s, byte mode ) {

  if( s->mode != mode ) {
    s->mode = mode;
  } else {
    s->spd = (s->spd + 1) % SPEED_STEPS;      
  }

}

void cycle_static_mode( State* s, byte hue ) {

  if( s->mode < STATIC_MODE_MIN ) {
    s->mode = STATIC_MODE_MIN;
  }

  if( s->col != hue ) {
    s->col = hue;
  } else {
    s->mode = (((s->mode - STATIC_MODE_MIN) + 1) % (MODE_NUM - STATIC_MODE_MIN)) + STATIC_MODE_MIN;
  }

}

/*
 * Maps from leds[] to real_leds[] using a mapping algorithm
 */
void mapleds(enum led_mapping_mode mapping) {
  switch(mapping) {

  case Z_UP: // no mapping
    for( int a = 0; a < NUM_LEDS; a++ ) {
      real_leds[a] = leds[a];
    }
    break;
  
  case Z_DOWN: // simple inverse
    for( int a = 0; a < NUM_LEDS; a++ ) {
      real_leds[NUM_LEDS - a - 1] = leds[a];
    }
    break;
  
  case MIRROR2: // both sides show the same (uses only every second element from leds[])
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[a*2];
      real_leds[a+(NUM_LEDS/2)] = leds[a*2];
    }
    break;
  
  case MIRROR2INV: // like MIRROR2, but starting from the other end (uses only every second element from leds[])
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[NUM_LEDS - (a*2) - 1];
      real_leds[a+(NUM_LEDS/2)] = leds[NUM_LEDS - (a*2) - 1];
    }
    break;
  
  case TWOWAY:  // like MIRROR2, but one side starts at the other end (uses only every second element from leds[])
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[a*2];
      real_leds[a+(NUM_LEDS/2)] = leds[NUM_LEDS - (a*2) - 1];
    }
    break;
  
  case TWOWAYINV: // like MIRROR2INV, but one side starts at the other end (uses only every second element from leds[])
    for( int a = 0; a < (NUM_LEDS/2); a++ ) {
      real_leds[a] = leds[NUM_LEDS - (a*2) - 1];
      real_leds[a+(NUM_LEDS/2)] = leds[a*2];
    }
    break;
  
  case MIRROR4: // 2-axis mirrored mapping center to border (uses only every fourth element from leds[])
    for( int a = 0; a < (NUM_LEDS*0.25); a++ ) {
      real_leds[(NUM_LEDS/4) - a - 1] = leds[a*4];
      real_leds[(int)(NUM_LEDS*0.75) - a - 1] = leds[a*4];
      real_leds[a + (NUM_LEDS/4)] = leds[a*4];
      real_leds[a + (int)(NUM_LEDS*0.75)] = leds[a*4];
    }
    break;
  
  case MIRROR4INV: // 2-axis mirrored mapping border to center (uses only every fourth element from leds[])
    for( int a = 0; a < (NUM_LEDS/4); a++ ) {
      real_leds[(NUM_LEDS/4) - a - 1] = leds[NUM_LEDS - (a*4) - 1];
      real_leds[(int)(NUM_LEDS*0.75) - a - 1] = leds[NUM_LEDS - (a*4) - 1];
      real_leds[a + (NUM_LEDS/4)] = leds[NUM_LEDS - (a*4) - 1];
      real_leds[a + (int)(NUM_LEDS*0.75)] = leds[NUM_LEDS - (a*4) - 1];
    }
    break;
  
  case CLOCK: // maps leds clockwise around
    for( int a = 0; a < (NUM_LEDS*0.5); a++ ) {
      real_leds[NUM_LEDS-a-1] = leds[a];
    }
    for( int a = (NUM_LEDS/2); a < NUM_LEDS; a++ ) {
      real_leds[a - (NUM_LEDS/2)] = leds[a];
    }
    break;
  
  case COUNTERCLOCK: // maps leds counterclockwise around
    for( int a = 0; a < (NUM_LEDS*0.5); a++ ) {
      real_leds[(NUM_LEDS/2)+a] = leds[a];
    }
    for( int a = (NUM_LEDS/2); a < NUM_LEDS; a++ ) {
      real_leds[NUM_LEDS-a-1] = leds[a];
    }
    break;
    
  }
}

/*
 * Reads state from EEPROM. If its no valid state (there is no MAGIC_NUMBER where it should be), defaults are being used.
 */
struct State read_state() {

  State s;
  EEPROM.get(STATE_ADDRESS, s);
  
  if( s.magic_number != MAGIC_NUMBER) {
    // invalid state found, setting defaults
    s.magic_number = MAGIC_NUMBER;
    s.bright = 1;         // 1/4 brightness
    s.spd = SPEED_STEPS/2;  // medium speed
    s.mode = 0;           // animated rainbow
    s.col = 0;            // red hue
    #ifdef DEBUG
    Serial.println("No valid state in EEPROM -- Using defaults.");
    #endif
  }
  #ifdef DEBUG
  else {
    Serial.println("Read valid state from EEPROM.");
  }
  print_state(&s);
  #endif

  return s;
}

/*
 * Saves state to EEPROM if the last user interaction was more than SAVE_GRACE_MS ms ago.
 * EEPROM.put also makes sure that a value is only written if it differs from the one already in EEPROM.
 */
void save_state(State s) {
  
  if( last_user_interaction > 0 && millis() > (last_user_interaction + SAVE_GRACE_MS) ) {
    
    EEPROM.put(STATE_ADDRESS, s);
    EEPROM.commit();
    // set last_user_interaction to 0, which is a special value that says "it has been saved already"
    last_user_interaction = 0;

    #ifdef DEBUG
    Serial.println("Saved state to EEPROM");
    #endif
  }
  
}

long map(long x, long in_min, long in_max, long out_min, long out_max) {
  return (double)(x - in_min) * (double)(out_max - out_min) / (double)(in_max - in_min) + (double)out_min;
}

/*
 * Debugging helper routines
 */
#ifdef DEBUG
// Display current state
void print_state(State* s) {
  Serial.print("Current state:\tbrightness: ");
  Serial.print(s->bright);
  Serial.print("\tmode: ");
  Serial.print(s->mode);
  Serial.print(" \tspeed: ");
  Serial.print(s->spd);
  Serial.print("\tcolor: ");
  Serial.println(s->col);
}
#endif
