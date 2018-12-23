
#include "FastLED.h"
#include "buttons.h"

// How many leds in your strip?
#define NUM_LEDS 24
// Where is the LED data line connected?
#define LED_DATA 7
#define BRIGHT_BTN 6
#define SPEED_BTN 5
#define MODE_BTN 4
#define IR_PIN 3
#define IR_INT 1

#define DEBUG

Button bright_btn, mode_btn, speed_btn;
byte bright = 0;
byte mode = 0;

enum led_mapping_mode { Z_UP, Z_DOWN, MIRROR2, MIRROR2INV, TWOWAY, TWOWAYINV, MIRROR4, MIRROR4INV, CLOCK, COUNTERCLOCK };
enum led_mapping_mode mapmode = Z_UP;
#define MODE_NUM 11

// IR globals
volatile unsigned long micro = 0;
volatile unsigned long lastmicro = 0;
volatile unsigned int IR_buffer[64];
volatile unsigned int len = 0;
volatile unsigned int lastlen = 0;
volatile short pos = -1;  
volatile unsigned long lastcode = 0;

// Define the array of leds
CRGB real_leds[NUM_LEDS];
CRGB real_leds_old[NUM_LEDS];
CRGB leds[NUM_LEDS];

void setup() { 
  // Initialize the LEDs
  pinMode(LED_DATA, OUTPUT); 
  
  bright_btn.assign(BRIGHT_BTN); 
  mode_btn.assign(MODE_BTN); 
  speed_btn.assign(SPEED_BTN); 

  bright_btn.setMode(OneShotTimer);
  mode_btn.setMode(OneShotTimer);
  speed_btn.setMode(OneShotTimer); 

  FastLED.addLeds<WS2812B, LED_DATA, GRB>(real_leds, NUM_LEDS);
  FastLED.setBrightness(bright);

  pinMode(IR_PIN, INPUT);
  attachInterrupt(IR_INT, IR_record, CHANGE);
  
  #ifdef DEBUG
  Serial.begin(115200);
  #endif
}

void loop() { 
  uint32_t ms = millis();

  if( lastcode > 0 ) {
    #ifdef DEBUG
    Serial.print(lastcode, HEX);
    Serial.print("   \t");
    for( int a = 0; a < 64; a++ ) {
      Serial.print(IR_buffer[a]);
      Serial.print(" ");
    }
    Serial.println();
    #endif
    lastcode = 0;
  }
  
  if( bright_btn.check() == ON ) {
    bright = (bright + 1) % 5;
  }

  if( mode_btn.check() == ON ) {
    mode = (mode + 1) % MODE_NUM;
  }

  FastLED.setBrightness((0xFF >> (4-bright)) * (bright>0));

  switch( 0 ) {
    case 0:
    fill_rainbow(leds, NUM_LEDS, (ms >> 4) & 0xFF, 255/NUM_LEDS);
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
  mapleds( led_mapping_mode(mode) );
  boolean changed = false;
  for( int a = 0; a < NUM_LEDS; a++ ) {
    if( real_leds[a] != real_leds_old[a] ) {
      changed = true;
      real_leds_old[a] = real_leds[a];
    }
  }
  if( pos < 0 && changed ) {
    FastLED.show();
  }
  delay(20);
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
  case MIRROR4:
    for( int a = 0; a < (NUM_LEDS*0.25); a++ ) {
      real_leds[(NUM_LEDS/4) - a - 1] = leds[a*4];
      real_leds[(int)(NUM_LEDS*0.75) - a - 1] = leds[a*4];
      real_leds[a + (NUM_LEDS/4)] = leds[a*4];
      real_leds[a + (int)(NUM_LEDS*0.75)] = leds[a*4];
    }
    break;
  case MIRROR4INV:
    for( int a = 0; a < (NUM_LEDS/4); a++ ) {
      real_leds[(NUM_LEDS/4) - a - 1] = leds[NUM_LEDS - (a*4) - 1];
      real_leds[(int)(NUM_LEDS*0.75) - a - 1] = leds[NUM_LEDS - (a*4) - 1];
      real_leds[a + (NUM_LEDS/4)] = leds[NUM_LEDS - (a*4) - 1];
      real_leds[a + (int)(NUM_LEDS*0.75)] = leds[NUM_LEDS - (a*4) - 1];
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

// IR Timing constants: microseconds -- See also: Extended NEC IR Protocol
#define IR_INTRO_H_L 8400  // IR Prefix High Length, Lower bound
#define IR_INTRO_H_U 10000  // IR Prefix High Length, Upper bound
#define IR_INTRO_L_L 3600  // IR Prefix Low Timing, Lower bound
#define IR_INTRO_L_U 5200  // IR Prefix Low Timing, Upper bound
#define IR_BITPULSE_L 450  // IR Bit Intro Pulse Length, Lower bound
#define IR_BITPULSE_U 800  // IR Bit Intro Pulse Length, Upper bound
#define IR_BIT_0_L 350  // IR 0-Bit Pause Length, Lower bound
#define IR_BIT_0_U 800  // IR 0-Bit Pause Length, Upper bound
#define IR_BIT_1_L 1400  // IR 1-Bit Pause Length, Lower bound
#define IR_BIT_1_U 1900  // IR 1-Bit Pause Length, Upper bound

inline boolean between(int a, int x, int b) {
  return (x >= a) && (x <= b);
}

void IR_decode() {
  int len_even_H, len_odd_L, b;
  lastcode = 0;
  for( int a = 31; a >= 0; a-- ) {
    b = a*2;
    len_even_H = IR_buffer[b];
    len_odd_L  = IR_buffer[b + 1];
    if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_0_L,len_odd_L,IR_BIT_0_U) ) {
      //pass
    } 
    else if( between(IR_BITPULSE_L,len_even_H,IR_BITPULSE_U) && between(IR_BIT_1_L,len_odd_L,IR_BIT_1_U) ) {
      lastcode = lastcode | (unsigned long)1 << (31-a);
    } 
    else {
      lastcode = 0xFFFFFFFFL;
      return;
    }
  }
}

void IR_record() {
  micro = micros();
  len = int(micro - lastmicro);
  if( pos > 63 ) {
    pos = -1;
    noInterrupts();
    IR_decode();
    interrupts();
  } 
  else if( pos >= 0 ) {
    IR_buffer[pos++] = len;
  } 
  else if( between(IR_INTRO_H_L,lastlen,IR_INTRO_H_U) && between(IR_INTRO_L_L,len,IR_INTRO_L_U) ) {
    pos = 0;
  } 
  lastmicro = micro;
  lastlen = len;
}

#ifdef DEBUG

#endif

