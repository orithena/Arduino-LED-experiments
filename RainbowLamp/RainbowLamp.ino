#include <IRremote.h>
#include <IRremoteInt.h>

#include "FastLED.h"
#include "buttons.h"

#include <EEPROM.h>


// How many leds in your strip?
#define NUM_LEDS 24
// Where is the LED data line connected?
#define LED_DATA 7

// Where are the buttons connected?
#define BRIGHT_BTN 6
#define SPEED_BTN 5
#define MODE_BTN 4

// Where is the IR receiver connected?
#define IR_PIN 3

// Minimum Speed number (actually, milliseconds are divided by 2^speed, so low speed numbers equal fast animation)
#define MIN_SPEED 1
// How many speed steps?
#define SPEED_STEPS 8

// Color is increased by this value
#define COLOR_INC (255/15)

// Comment this in if you want to debug via Serial console.
//#define DEBUG

// Choose your remote
//#define CHEAP_STANDARD_CHINA_REMOTE
#define MCL_REMOTE

// Global Variables for the buttons
Button bright_btn, mode_btn, speed_btn;

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
enum led_mapping_mode { Z_UP, Z_DOWN, MIRROR2, MIRROR2INV, TWOWAY, TWOWAYINV, MIRROR4INV, MIRROR4, CLOCK, COUNTERCLOCK };

// Define the led arrays
// This is the led output array
CRGB real_leds[NUM_LEDS];
// This is a copy of the led output array, so we can check whether something has changed
CRGB real_leds_old[NUM_LEDS];
// This is our "working copy" of the led array; it's copied to real_leds by mapleds(led_mapping_mode)
CRGB leds[NUM_LEDS];

// Global variable for the IR receiver
IRrecv irrecv(IR_PIN);

// lets save whether something has changed so we know whether to update the LEDs.
boolean something_happened = false;

/*
 * This routine is run once at powerup.
 */
void setup() {
  #ifdef DEBUG
  // Initialize Serial console.
  delay(2000);
  Serial.begin(115200);
  #endif

  // read fom EEPROM
  state = read_state();
  // if powered up, we want to see something, so we increase brightness to at least 1
  if( state.bright == 0 ) {
    state.bright = 1;
  }
  
  // initialize LED data output pin.
  pinMode(LED_DATA, OUTPUT); 

  // initialize button routines.
  bright_btn.assign(BRIGHT_BTN); 
  bright_btn.setMode(OneShotTimer);
  mode_btn.assign(MODE_BTN); 
  mode_btn.setMode(OneShotTimer);
  speed_btn.assign(SPEED_BTN); 
  speed_btn.setMode(OneShotTimer); 

  // Start the IR receiver
  irrecv.enableIRIn();

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
  // let's save the current time, we'll need it often
  uint32_t ms = millis();

  // read IR and Button events. This may change state.
  something_happened |= interpret_IR(&state);
  something_happened |= interpret_Buttons(&state);

  // Now use the state to show something :)

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
    fill_gradient_RGB(leds, NUM_LEDS, CHSV(state.col,255,0), CHSV(state.col,255,255));
    mapleds( MIRROR4 );
    break;
    case 6:   // mode 6 is a static color gradient, mapped using TWOWAY
    fill_gradient_RGB(leds, NUM_LEDS, CHSV(state.col,255,0), CHSV(state.col,255,255));
    mapleds( TWOWAY );
    break;
    case 7:
    case 8:   // mode 7 and 8 is a static rainbow starting at state.col, mapped using Z_UP(7) or COUNTERCLOCK(8)
    fill_rainbow(leds, NUM_LEDS, state.col, 256/NUM_LEDS);
    mapleds( (state.mode-7)*((int)(COUNTERCLOCK)) );
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

  // if the led array has changed and there's currently no activity on IR, we actually update the LEDs.
  // (IR relies on interrupts and FastLED.show() temporarily disables interrupts, so we need to check that.)
  if( something_happened && irrecv.isIdle() ) {
    FastLED.show();
    // reset the event marker
    something_happened = false;
  }

  // save state to EEPROM (the routine checks whether its necessary)
  save_state(state);

  // wait a bit. We've got a bit of time, actually :)
  // 10 ms waiting time translates to an update frequency of ~100Hz. That is enough for human eyes :)
  delay(10);
}

/*
 * Check whether one of the three hardware buttons has been pressed
 * and change state accordinly.
 */
boolean interpret_Buttons(State* state) {
  boolean something_was_pressed = false;

  if( bright_btn.check() == ON ) {
    if( power_state == false ) {
      power_state = true;
      if( state->bright == 0 ) {
        state->bright = 1;
      }
    } else {
      state->bright = (state->bright + 1) % 5;
    }
    last_user_interaction = millis();
    something_was_pressed = true;
    #ifdef DEBUG
    Serial.print("Got brightness button. \t");
    print_state(state);
    #endif
  }

  if( mode_btn.check() == ON ) {
    if( power_state ) {
      state->mode = (state->mode + 1) % MODE_NUM;
    } else {
      power_state = true;
    }
    last_user_interaction = millis();
    something_was_pressed = true;
    #ifdef DEBUG
    Serial.print("Got mode button.       \t");
    print_state(state);
    #endif
  }

  if( speed_btn.check() == ON ) {
    if( power_state ) {
      if( state->mode < STATIC_MODE_MIN ) {
        // in animated modes, this button changes speed.
        state->spd = (state->spd + 1) % SPEED_STEPS;
      } else {
        // in static modes, this button changes color.
        // this formula makes sure that the colors are set to a multiple of COLOR_INC while increasing the hue by color_inc
        state->col = (byte)((state->col - (state->col % COLOR_INC) + COLOR_INC) % (256 - (256 % COLOR_INC)));
      }
    }
    last_user_interaction = millis();
    something_was_pressed = true;
    #ifdef DEBUG
    Serial.print("Got speed/color button.\t");
    print_state(state);
    #endif
  }
  return something_was_pressed;
}

/*
 * Check whether there was an IR event and change state accordingly.
 */
boolean interpret_IR(State* state) {
  decode_results ir_results;        // Somewhere to store the results

  if (irrecv.decode(&ir_results)) {  // Grab an IR code
    #ifdef DEBUG
    Serial.print("Got IR event. \t");
    dumpInfo(&ir_results);
    #endif

    #ifdef CHEAP_STANDARD_CHINA_REMOTE
    if( ir_results.decode_type == NEC ) {
      switch( ir_results.value ) {
        case 0x00F700FF:    // brightness up
          state->bright = (state->bright + 1) % 5;
          break;
        case 0x00F7807F:    // brightness down
          state->bright = (state->bright + 4) % 5;
          break;
        case 0x00F7C03F:    // power on
          power_state = true;
          if( state->bright == 0 ) {
            state->bright = 1;
          }
          break;
        case 0x00F740BF:    // power off
          power_state = false;
          break;
        case 0x00F7D02F:    // flash
          cycle_speed(state, 0);
          break;
        case 0x00F7F00F:    // strobe
          cycle_speed(state, 1);
          break;
        case 0x00F7C837:    // fade
          cycle_speed(state, 2);
          break;
        case 0x00F7E817:    // smooth
          cycle_speed(state, 3);
          break;
        case 0x00F7E01F:    // W
          cycle_speed(state, 4);
          break;
        case 0x00F720DF:    // R
          cycle_static_mode(state, 0);
          break;
        case 0x00F710EF:    // R1
          cycle_static_mode(state, 17);
          break;
        case 0x00F730CF:    // R2
          cycle_static_mode(state, 34);
          break;
        case 0x00F708F7:    // R3
          cycle_static_mode(state, 51);
          break;
        case 0x00F728D7:    // R4
          cycle_static_mode(state, 68);
          break;
        case 0x00F7A05F:    // G
          cycle_static_mode(state, 85);
          break;
        case 0x00F7906F:    // G1
          cycle_static_mode(state, 102);
          break;
        case 0x00F7B04F:    // G2
          cycle_static_mode(state, 119);
          break;
        case 0x00F78877:    // G3
          cycle_static_mode(state, 136);
          break;
        case 0x00F7A857:    // G4
          cycle_static_mode(state, 153);
          break;
        case 0x00F7609F:    // B
          cycle_static_mode(state, 170);
          break;
        case 0x00F750AF:    // B1
          cycle_static_mode(state, 187);
          break;
        case 0x00F7708F:    // B2
          cycle_static_mode(state, 204);
          break;
        case 0x00F748B7:    // B3
          cycle_static_mode(state, 221);
          break;
        case 0x00F76897:    // B4
          cycle_static_mode(state, 238);
          break;
      }
    }
    #endif

    #ifdef MCL_REMOTE
    if( ir_results.decode_type == NEC ) {
      switch( ir_results.value ) {
        case 0x00FF609F:    // ON
          power_state = true;
          if( state->bright == 0 ) {
            state->bright = 1;
          }
          break;
        case 0x00FF807F:    // OFF
          power_state = false;
          break;
        case 0x00FF08F7:    // Setup
          state->spd = (state->spd + SPEED_STEPS - 1) % SPEED_STEPS;
          break;
        case 0x00FFC03F:    // Cancel
          state->spd = (state->spd + 1) % SPEED_STEPS;
          break;
        case 0x00FF906F:    // Flash
          cycle_speed(state, 0);
          break;
        case 0x00FFB847:    // Strobe
          cycle_speed(state, 1);
          break;
        case 0x00FFF807:    // Fade
          cycle_speed(state, 2);
          break;
        case 0x00FFB04F:    // Smooth
          cycle_speed(state, 3);
          break;
        case 0x00FFA857:    // W
          cycle_speed(state, 4);
          break;
        case 0x00FF9867:    // R
          cycle_static_mode(state, 0);
          break;
        case 0x00FFE817:    // R1
          cycle_static_mode(state, 17);
          break;
        case 0x00FF02FD:    // R2
          cycle_static_mode(state, 34);
          break;
        case 0x00FF40BF:    // R3
          cycle_static_mode(state, 51);
          break;
        case 0x00FF50AF:    // R4
          cycle_static_mode(state, 68);
          break;
        case 0x00FFD827:    // G
          cycle_static_mode(state, 85);
          break;
        case 0x00FF48B7:    // G1
          cycle_static_mode(state, 102);
          break;
        case 0x00FF12ED:    // G2
          cycle_static_mode(state, 119);
          break;
        case 0x00FFA05F:    // G3
          cycle_static_mode(state, 136);
          break;
        case 0x00FF7887:    // G4
          cycle_static_mode(state, 153);
          break;
        case 0x00FF8877:    // B
          cycle_static_mode(state, 170);
          break;
        case 0x00FF6897:    // B1
          cycle_static_mode(state, 187);
          break;
        case 0x00FF20DF:    // B2
          cycle_static_mode(state, 204);
          break;
        case 0x00FF2AD5:    // B3
          cycle_static_mode(state, 221);
          break;
        case 0x00FF708F:    // B4
          cycle_static_mode(state, 238);
          break;
        case 0x00FF32CD:    // Bright 4
          state->bright = 4;
          break;
        case 0x00FF00FF:    // Bright 3
          state->bright = 3;
          break;
        case 0x00FFB24D:    // Bright 2
          state->bright = 2;
          break;
        case 0x00FF58A7:    // Bright 1
          state->bright = 1;
          break;
      }
    }
    #endif
      
    // save user interaction timestamp
    last_user_interaction = millis();
    #ifdef DEBUG
    print_state(state);
    #endif
    irrecv.resume();              // Prepare for the next value
    return true;
  }
  return false;
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
    // set last_user_interaction to 0, which is a special value that says "it has been saved already"
    last_user_interaction = 0;

    #ifdef DEBUG
    Serial.println("Saved state to EEPROM");
    #endif
  }
  
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

// Display IR code
void ircode (decode_results *results)
{
  // Panasonic has an Address
  if (results->decode_type == PANASONIC) {
    Serial.print(results->address, HEX);
    Serial.print(":");
  }

  // Print Code
  Serial.print(results->value, HEX);
}

// Display IR encoding type
void encoding (decode_results *results)
{
  switch (results->decode_type) {
    default:
    case UNKNOWN:      Serial.print("UNKNOWN");       break ;
    case NEC:          Serial.print("NEC");           break ;
    case SONY:         Serial.print("SONY");          break ;
    case RC5:          Serial.print("RC5");           break ;
    case RC6:          Serial.print("RC6");           break ;
    case DISH:         Serial.print("DISH");          break ;
    case SHARP:        Serial.print("SHARP");         break ;
    case JVC:          Serial.print("JVC");           break ;
    case SANYO:        Serial.print("SANYO");         break ;
    case MITSUBISHI:   Serial.print("MITSUBISHI");    break ;
    case SAMSUNG:      Serial.print("SAMSUNG");       break ;
    case LG:           Serial.print("LG");            break ;
    case WHYNTER:      Serial.print("WHYNTER");       break ;
    case AIWA_RC_T501: Serial.print("AIWA_RC_T501");  break ;
    case PANASONIC:    Serial.print("PANASONIC");     break ;
    case DENON:        Serial.print("Denon");         break ;
  }
}

// Dump out the IR decode_results structure.
void dumpInfo (decode_results *results)
{
  // Check if the buffer overflowed
  if (results->overflow) {
    Serial.println("IR code too long.");
    return;
  }

  // Show Encoding standard
  Serial.print("Encoding: ");
  encoding(results);

  // Show Code & length
  Serial.print("\tCode: ");
  ircode(results);
  Serial.print(" (");
  Serial.print(results->bits, DEC);
  Serial.println(" bits)");
}
#endif

