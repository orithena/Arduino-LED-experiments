#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 16
// Where is the LED data line connected?
#define DATA_PIN 2

// Define the array of leds
CRGB leds[NUM_LEDS];

#define MODE_COUNT 5

void setup() { 
  // Initialize the LEDs
  pinMode(DATA_PIN, OUTPUT); 
  FastLED.addLeds<APA106, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

void loop() {
  uint32_t ms = millis();
  int s = ms >> 10;
  int mode = (s >> 5) % MODE_COUNT;
  switch(mode) {
    case 0:
      loop_Text(ms);
      break;
    case 1:
      loop_Rainbow(ms);
      break;
    case 2:
      loop_Rainbow_Inc(ms);
      break;
    case 3:
      loop_RGBBlink(ms);
      break;
    case 4:
      loop_Rainbow2(ms);
      break;
    case 5:
      loop_RGB_sawtooth(ms);
      break;
  }
  delay(10);
}

void Clear() {
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(0,0,0);
  }
}

void loop_RGBBlink(uint32_t ms) { 
  int col = (ms >> 1);
  for( int i = 0; i < NUM_LEDS; i++ ) {
    
    leds[i] = CHSV(col,255,255 * ((ms >> 6) % 2));

  }
  
  FastLED.show();
}

void loop_WhiteBlink(uint32_t ms) { 
  for( int i = 0; i < NUM_LEDS; i++ ) {
    
    leds[i] = CHSV(60,0,160 * ((ms >> 6) % 2));

  }
  
  FastLED.show();
}

void loop_Rainbow2(uint32_t ms) { 
  Clear();
  int col = (ms);
  for( int i = 0; i < NUM_LEDS/2; i++ ) {
    
    leds[i << 1] = CHSV(col + i*(256/NUM_LEDS),255,255);
    leds[(i << 1)+1] = CHSV(col + (NUM_LEDS - i)*(256/NUM_LEDS),255,255);

  }
  
  FastLED.show();
}

void loop_Rainbow(uint32_t ms) { 
  Clear();
  int col = (ms >> 1);
  for( int i = 0; i < NUM_LEDS/2; i++ ) {
    
    leds[i] = CHSV(col + i*(128/NUM_LEDS),255,255);
    leds[NUM_LEDS - 1 - i] = CHSV(col + i*(128/NUM_LEDS),255,255);

  }
  
  FastLED.show();
}

void loop_Rainbow_Inc(uint32_t ms) { 
  Clear();
  int col = (ms >> 8) % NUM_LEDS;
  for( int i = 0; i <= col; i++ ) {
    
    leds[i] = CHSV(col + i*(255/NUM_LEDS),255,255);

  }
  
  FastLED.show();
}

void loop_RGB_sawtooth(uint32_t ms) { 
  Clear();
  int count = (ms >> 3) % (NUM_LEDS/2);
  int col = ((ms >> 3) / (NUM_LEDS/2)) % 3;
  for( int i = 0; i <= count; i++ ) {
    
    leds[i] = CHSV(col*85,255,255);
    leds[NUM_LEDS - 1 - i] = CHSV(col*85,255,255);

  }
  
  FastLED.show();
}

static unsigned char Font5x7[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,// (space)
  0x00, 0x00, 0x5F, 0x00, 0x00,// !
  0x00, 0x07, 0x00, 0x07, 0x00,// "
  0x14, 0x7F, 0x14, 0x7F, 0x14,// #
  0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
  0x23, 0x13, 0x08, 0x64, 0x62,// %
  0x36, 0x49, 0x55, 0x22, 0x50,// &
  0x00, 0x05, 0x03, 0x00, 0x00,// '
  0x00, 0x1C, 0x22, 0x41, 0x00,// (
  0x00, 0x41, 0x22, 0x1C, 0x00,// )
  0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
  0x08, 0x08, 0x3E, 0x08, 0x08,// +
  0x00, 0x50, 0x30, 0x00, 0x00,// ,
  0x08, 0x08, 0x08, 0x08, 0x08,// -
  0x00, 0x60, 0x60, 0x00, 0x00,// .
  0x20, 0x10, 0x08, 0x04, 0x02,// /
  0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
  0x00, 0x42, 0x7F, 0x40, 0x00,// 1
  0x42, 0x61, 0x51, 0x49, 0x46,// 2
  0x21, 0x41, 0x45, 0x4B, 0x31,// 3
  0x18, 0x14, 0x12, 0x7F, 0x10,// 4
  0x27, 0x45, 0x45, 0x45, 0x39,// 5
  0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
  0x01, 0x71, 0x09, 0x05, 0x03,// 7
  0x36, 0x49, 0x49, 0x49, 0x36,// 8
  0x06, 0x49, 0x49, 0x29, 0x1E,// 9
  0x00, 0x36, 0x36, 0x00, 0x00,// :
  0x00, 0x56, 0x36, 0x00, 0x00,// ;
  0x00, 0x08, 0x14, 0x22, 0x41,// <
  0x14, 0x14, 0x14, 0x14, 0x14,// =
  0x41, 0x22, 0x14, 0x08, 0x00,// >
  0x02, 0x01, 0x51, 0x09, 0x06,// ?
  0x32, 0x49, 0x79, 0x41, 0x3E,// @
  0x7E, 0x11, 0x11, 0x11, 0x7E,// A
  0x7F, 0x49, 0x49, 0x49, 0x36,// B
  0x3E, 0x41, 0x41, 0x41, 0x22,// C
  0x7F, 0x41, 0x41, 0x22, 0x1C,// D
  0x7F, 0x49, 0x49, 0x49, 0x41,// E
  0x7F, 0x09, 0x09, 0x01, 0x01,// F
  0x3E, 0x41, 0x41, 0x51, 0x32,// G
  0x7F, 0x08, 0x08, 0x08, 0x7F,// H
  0x00, 0x41, 0x7F, 0x41, 0x00,// I
  0x20, 0x40, 0x41, 0x3F, 0x01,// J
  0x7F, 0x08, 0x14, 0x22, 0x41,// K
  0x7F, 0x40, 0x40, 0x40, 0x40,// L
  0x7F, 0x02, 0x04, 0x02, 0x7F,// M
  0x7F, 0x04, 0x08, 0x10, 0x7F,// N
  0x3E, 0x41, 0x41, 0x41, 0x3E,// O
  0x7F, 0x09, 0x09, 0x09, 0x06,// P
  0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
  0x7F, 0x09, 0x19, 0x29, 0x46,// R
  0x46, 0x49, 0x49, 0x49, 0x31,// S
  0x01, 0x01, 0x7F, 0x01, 0x01,// T
  0x3F, 0x40, 0x40, 0x40, 0x3F,// U
  0x1F, 0x20, 0x40, 0x20, 0x1F,// V
  0x7F, 0x20, 0x18, 0x20, 0x7F,// W
  0x63, 0x14, 0x08, 0x14, 0x63,// X
  0x03, 0x04, 0x78, 0x04, 0x03,// Y
  0x61, 0x51, 0x49, 0x45, 0x43,// Z
  0x00, 0x00, 0x7F, 0x41, 0x41,// [
  0x02, 0x04, 0x08, 0x10, 0x20,// "\"
  0x41, 0x41, 0x7F, 0x00, 0x00,// ]
  0x04, 0x02, 0x01, 0x02, 0x04,// ^
  0x40, 0x40, 0x40, 0x40, 0x40,// _
  0x00, 0x01, 0x02, 0x04, 0x00,// `
  0x20, 0x54, 0x54, 0x54, 0x78,// a
  0x7F, 0x48, 0x44, 0x44, 0x38,// b
  0x38, 0x44, 0x44, 0x44, 0x20,// c
  0x38, 0x44, 0x44, 0x48, 0x7F,// d
  0x38, 0x54, 0x54, 0x54, 0x18,// e
  0x08, 0x7E, 0x09, 0x01, 0x02,// f
  0x08, 0x14, 0x54, 0x54, 0x3C,// g
  0x7F, 0x08, 0x04, 0x04, 0x78,// h
  0x00, 0x44, 0x7D, 0x40, 0x00,// i
  0x20, 0x40, 0x44, 0x3D, 0x00,// j
  0x00, 0x7F, 0x10, 0x28, 0x44,// k
  0x00, 0x41, 0x7F, 0x40, 0x00,// l
  0x7C, 0x04, 0x18, 0x04, 0x78,// m
  0x7C, 0x08, 0x04, 0x04, 0x78,// n
  0x38, 0x44, 0x44, 0x44, 0x38,// o
  0x7C, 0x14, 0x14, 0x14, 0x08,// p
  0x08, 0x14, 0x14, 0x18, 0x7C,// q
  0x7C, 0x08, 0x04, 0x04, 0x08,// r
  0x48, 0x54, 0x54, 0x54, 0x20,// s
  0x04, 0x3F, 0x44, 0x40, 0x20,// t
  0x3C, 0x40, 0x40, 0x20, 0x7C,// u
  0x1C, 0x20, 0x40, 0x20, 0x1C,// v
  0x3C, 0x40, 0x30, 0x40, 0x3C,// w
  0x44, 0x28, 0x10, 0x28, 0x44,// x
  0x0C, 0x50, 0x50, 0x50, 0x3C,// y
  0x44, 0x64, 0x54, 0x4C, 0x44,// z
  0x00, 0x08, 0x36, 0x41, 0x00,// {
  0x00, 0x00, 0x7F, 0x00, 0x00,// |
  0x00, 0x41, 0x36, 0x08, 0x00,// }
  0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
  0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
};

unsigned char* Char(unsigned char* font, char c) {
  return &font[(c - 32) * 5];
}

void DrawSprite(unsigned char* sprite, int xOffset) {
  if( xOffset > 4 ) { return; }
  for ( byte y = 0; y < 7; y++) {
      bool on = (sprite[xOffset] >> (6 - y) & 1);
      if (on) {
        leds[ (NUM_LEDS/2) - 1 - y ]  = CHSV( 170, 255, 255 );
        leds[ (NUM_LEDS/2) + y ]  = CHSV( 170, 255, 255 );
      }
  }
}

void DrawText(char *text, int x) {
  DrawSprite(Char(Font5x7, text[x/6]), x%6);
}

uint32_t textcounter = 0;
void loop_Text(uint32_t ms) {
  Clear();
  DrawText("HALLO!", textcounter % 36);
  FastLED.show();
  textcounter++;
}
