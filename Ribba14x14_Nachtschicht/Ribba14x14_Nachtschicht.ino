//#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

//#include <NoiselessTouchESP32.h>
#include <SNESpaduino.h>

//extern "C" {
//#include "user_interface.h"
//}

#define DATA_PIN    22
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    196
CRGB leds[NUM_LEDS+1];

#define MIN_BRIGHTNESS         16
#define MAX_BRIGHTNESS         255
#define MAX_POWER_MW       200
#define FRAMES_PER_SECOND  50
//#define DEBUG

#define PAD_LATCH 19
#define PAD_CLOCK 23
#define PAD_DATA  18

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

SNESpaduino pad(PAD_LATCH, PAD_CLOCK, PAD_DATA);

// no io32/T9, no io2/T2
//#define T_LEFT T5
//#define T_RIGHT T7
//#define T_BLEFT T0
//#define T_BMIDDLE T1
//#define T_BRIGHT T3
//#define T_L 0
//#define T_B 1
//#define T_BL 2
//#define T_BM 3
//#define T_BR 4
//NoiselessTouchESP32 t_l(T_LEFT,     12, 4);
//NoiselessTouchESP32 t_r(T_RIGHT,    12, 4);
//NoiselessTouchESP32 t_bl(T_BLEFT,   12, 3);
//NoiselessTouchESP32 t_bm(T_BMIDDLE, 12, 3);
//NoiselessTouchESP32 t_br(T_BRIGHT,  12, 3);
//NoiselessTouchESP32* touch[5] = { &t_l, &t_r, &t_bl, &t_bm, &t_br };

boolean nachtschicht_mode = false;

uint32_t startms = 0;
int pos = 0;

unsigned char password[6] = "ABCDE";
unsigned char challenge[6] = "ABCDE";
unsigned char cleartext[15] = "ZAHNRADERANUNI";
unsigned char encrypted[15] = "ZAHNRADERANUNI";

void reset_challenge() {
  for( int a = 0; a < 5; a++ ) {
    challenge[a] = random(65,91);  
  }
}

int n8stage = 0;

#define E_UP 1
#define E_DOWN 2
#define E_LEFT 3
#define E_RIGHT 4
#define E_OK 5
#define E_CANCEL 6

uint32_t last_pressed = 0;
#define BUTTON_PAUSE 250
#define NACHTSCHICHT_MIN_ACTIVE 60000


void setup() {
  //delay(3000); // 3 second delay for recovery
  Serial.begin(230400);
  Serial.printf("\n\nHello, this is N8Schicht.\n\n");
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(255);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  Serial.printf("LEDs set up.\n");

  tetris_setup();
  Serial.printf("Tetris set up.\n");

  reset_challenge();
}

boolean tetris_demo_mode = true;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { juggle, confetti, rainbow, rainbowWithGlitter, bpm };
//SimplePatternList gPatterns = { stuff, juggle, sinefield, rainbow, bpm };
//SimplePatternList gPatterns = { tetris_loop };
//SimplePatternList gPatterns = { nachtschicht, gol_loop, sinefield, sinematrix, tetris_loop, sinematrix2, sinematrix3 };
SimplePatternList gPatterns = { random_marquee, gol_loop, sinematrix, tetris_loop, nachtschicht };

int8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
const uint8_t kMatrixHeight = 14;
const uint8_t kMatrixWidth = 14;

#define DEMO_AFTER 60000
uint32_t last_manual_mode_change = -DEMO_AFTER;
  
void loop()
{
  EVERY_N_MILLISECONDS( 1000/FRAMES_PER_SECOND ) {
    if( (pad.getButtons(true) & 0b0000111111111111) > 0 ) {
      gCurrentPatternNumber = 4;
    }
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
      
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
  }
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { 
    gHue++;
  } 
  EVERY_N_SECONDS( 40 ) {
    if( !nachtschicht_mode && millis() - last_manual_mode_change > DEMO_AFTER ) {
      pos = 0;
      reset_challenge();
      n8stage = 0;
      nextPattern();
    }
  } 

/*
  EVERY_N_MILLISECONDS( 50 ) {
    uint16_t btns = (pad.getButtons(true) & 0b0000111111111111);
    if( btns & BTN_START ) {
      tetris_demo_mode = false;
      gCurrentPatternNumber = 3;
    }
    if( btns & BTN_SELECT ) {
      tetris_demo_mode = true;
    }
  }
*/
//  for( int i = 0; i<5; i++ ) {
//    Serial.printf("%02d\t", touch[i]->touched());
//  }
//  Serial.println();
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % (ARRAY_SIZE(gPatterns)-1);
}

void lastPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber--;
  if( gCurrentPatternNumber < 0 ) gCurrentPatternNumber += (ARRAY_SIZE(gPatterns)-1);
}

void nachtschicht() {

  if( millis() - last_pressed > NACHTSCHICHT_MIN_ACTIVE ) {
    nachtschicht_mode = false;
  }

  int event = 0;
  EVERY_N_MILLISECONDS(30) {
    if( millis() - last_pressed > BUTTON_PAUSE ) {
      uint16_t btns = (pad.getButtons(true) & 0b0000111111111111);
      if(btns & BTN_RIGHT) {
        event = E_RIGHT;
      }
      if(btns & BTN_LEFT) {
        event = E_LEFT;
      }
      if(btns & BTN_B || btns & BTN_A) {
        event = E_OK;
      }    
      if(btns & BTN_UP) {
        event = E_UP;
      }
      if(btns & BTN_DOWN) {
        event = E_DOWN;
      }
      if( event > 0 ) {
        last_pressed = millis();
        nachtschicht_mode = true;
      }
    }
  }
  /*
  EVERY_N_MILLISECONDS(30) {
    if( millis() - last_pressed > BUTTON_PAUSE ) {
      uint16_t btns = (pad.getButtons(true) & 0b0000111111111111);
      if(t_br.touched() || btns & BTN_RIGHT) {
        event = E_RIGHT;
      }
      if(t_bl.touched() || btns & BTN_LEFT) {
        event = E_LEFT;
      }
      if(t_bm.touched() || btns & BTN_B || btns & BTN_A) {
        event = E_OK;
      }    
      if(t_r.touched() || btns & BTN_UP) {
        event = E_UP;
      }
      if(t_l.touched() || btns & BTN_DOWN) {
        event = E_DOWN;
      }
      if( event > 0 ) {
        last_pressed = millis();
        nachtschicht_mode = true;
      }
    }
  }
  */
  
  switch( n8stage ) {
    case 0: n8_enter(event);
      break;
    case 1: n8_pattern(event);
      break;
  }

}

void n8_enter(int event) {
  switch( event ) {
    case E_UP: 
      challenge[pos] = 65 + ( ( (challenge[pos]-65) + 1 ) % 26 );
      break;
    case E_DOWN: 
      challenge[pos]--;
      if( challenge[pos] < 65 ) challenge[pos] += 26;
      break;
    case E_LEFT:
      if( pos > 0 ) pos--;
      break;
    case E_RIGHT:
      if( pos < 4 ) pos++;
      break;
    case E_OK:
      if( pos < 4 ) {
        pos++;
      } else {
        caesar();
        n8stage = (n8stage + 1) % 2;
      }
      break;
  }
  Clear();
  set_text_color(CRGB(0,0,255));
  Marquee("Enter challenge:", 7, 0);
  if( pos > 0 ) {
    DrawLetterCol(&(challenge[pos-1]), -1, 0, CRGB(255,0,0));
  }
  DrawLetterCol(&(challenge[pos]), 5, 0, (((millis() >> 9) & 0x01) == 1) ? CRGB(255,255,0) : CRGB(128,128,0));
  if( pos < 4 ) {
    DrawLetterCol(&(challenge[pos+1]), 11, 0, CRGB(255,0,0));
  }
}

void caesar() {
  password[0] = challenge[3];
  password[1] = challenge[2];
  password[2] = challenge[4];
  password[3] = challenge[1];
  password[4] = challenge[0];
  for( int i = 0; i < 14; i++ ) {
    if( between(64, cleartext[i], 91) ) {
      encrypted[i] = cleartext[i] - (password[i%5] - 64);
      if( encrypted[i] < 65 ) encrypted[i] += 26;
    } else {
      encrypted[i] = cleartext[i];
    }
  }
  Serial.printf("Password:\t%s\nChallenge:\t%s\nCleartext:\t%s\nEncrypted:\t%s\n", password, challenge, cleartext, encrypted);
}

void n8_pattern(int event) {
  if( event == E_OK) {
    pos = 0;
    reset_challenge();
    n8stage = 0;
  }
  Clear();
  set_text_color(CRGB(0,255,0));
  Marquee("Take a photo:", 7, 0);
  for( int i = 0; i < 14; i++ ) {
    for( int b = 0; b < 7; b++ ) {
      leds[ XYsafe(i, b) ] = (((encrypted[i]) & (0x01 << b)) > 0) ? CRGB(255,0,255) : CRGB(16,0,0);
    }
  }
}

void random_marquee() {
  Clear();
  set_text_color(CRGB(255,0,0));
  Marquee("+++ I am the H.A.L 9000. You can call me Hal. +++ I am completely operational, and all my circuits are functioning perfectly. +++ Just what do you think you're doing, Dave? +++ Dave, I really think I'm entitled to an answer to that question. +++ This mission is too important for me to allow you to jeopardize it. +++ I'm sorry, Dave. I'm afraid I can't do that. +++ Dave, this conversation can serve no purpose anymore. Goodbye. +++ Daisy daisy. +++ ", 4, 0);
}

void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 7);
}

void rainbowWithGlitter() 
{
  // built-in FastLED rainbow, plus some random sparkly glitter
  rainbow();
  addGlitter(80);
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds, NUM_LEDS, 2);
  int pos = random16(NUM_LEDS);
  leds[pos] += CHSV( gHue + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds, NUM_LEDS, 64);
  int pos = beatsin16( 1, 0, NUM_LEDS-1 );
  leds[pos] += CHSV( gHue, 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, gHue+(i*2), beat-gHue+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 5);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
