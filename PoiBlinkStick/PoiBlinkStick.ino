#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 12
#define ADD_LEDS 3
// Where is the LED data line connected?
#define DATA_PIN 6
#define BUTTON_PIN 8

// Define the array of leds
CRGB real_leds[NUM_LEDS+ADD_LEDS];
CRGB* leds = real_leds + ADD_LEDS;
uint32_t last_button = 0;
int user_mode = 0;

#define MODE_COUNT 10
#define USER_MODE_COUNT 11

void setup() { 
  // Initialize the LEDs
  pinMode(DATA_PIN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  FastLED.addLeds<APA106, DATA_PIN, RGB>(real_leds, NUM_LEDS+ADD_LEDS);
  FastLED.setBrightness(255);
  last_button = millis();
}

void loop() {
  uint32_t ms = millis();
  if( check_button() ) {
    user_mode = (user_mode + 1) % USER_MODE_COUNT;
  }
  int s = ms >> 10;
  int mode = 0;
  switch(user_mode) {
    case 0:
      mode = (s >> 5) % MODE_COUNT;
      break;
    default:
      mode = user_mode;
      break;
  }
  switch(mode) {
    case 1:
      loop_Pacman(ms);
      base_Color(CRGB(255,255,0));
      break;
    case 2:
      loop_RGB_sawtooth(ms);
      base_RGB(ms);
      break;
    case 3:
      loop_Rainbow(ms);
      base_RGB(ms);
      break;
    case 4:
      loop_Rainbow2(ms);
      base_RGB(ms);
      break;
    case 5:
      loop_RGBBlink(ms);
      base_ColorBlink(ms, CRGB(255,255,255));
      break;
    case 6:
      loop_Rainbow_Inc(ms);
      base_RGB(ms);
      break;
    case 7:
      loop_Red(ms);
      base_Color(CRGB(255,0,0));
      break;
    case 8:
      loop_Heart(ms);
      base_Color(CRGB(255,0,0));
      break;
    case 9:
      loop_Pong(ms);
      base_RGB(ms);
      break;
    case 10:
      loop_Text(ms);
      base_Color(CRGB(0,0,255));
      break;
    default:
      Clear();
      base_RGB(ms);
      break;
  }
  FastLED.show();
  delay(1);
}

void base_RGB(uint32_t ms) {
  byte x = ((ms >> 5) & 0x00FF) % 3;
  for( int i = 0; i < ADD_LEDS; i++ ) {
    real_leds[i] = CRGB(255 * (x == i), 255 * (x==((i+1)%3)), 255 * (x==((i+2)%3)));
  }
}

void base_Color(CRGB col) {
  for( int i = 0; i < ADD_LEDS; i++ ) {
    real_leds[i] = col;
  }
}

void base_ColorBlink(uint32_t ms, CRGB col) {
  for( int i = 0; i < ADD_LEDS; i++ ) {
    byte x = ((ms >> 2) & 0x01);
    real_leds[i] = CRGB(col.r*x, col.g*x, col.b*x);
  }
}

void Clear() {
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CRGB(0,0,0);
  }
}  


static uint16_t heart[] = {
  0b0000001110000000,
  0b0000011111100000,
  0b0000111111111000,
  0b0000111111111100,
  0b0000001111111111,
  
  0b0000111111111100,
  0b0000111111111000,
  0b0000011111100000,
  0b0000001110000000,
  0b0000000000000000
};

void loop_Heart(uint32_t ms) {
  int idx = (((ms>>3) & 0x0FFF) % 10);
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255 * ((heart[idx] >> (NUM_LEDS - 1 - i)) & 0x0001),0,0);
  }  
}


static uint32_t pacman[] = {
  //_F_E_D_C_B_A_9_8_7_6_5_4_3_2_1_0
  0b00000000000000001010101000000000,
  0b00000000000010101010101010100000,
  0b00000000001010101010101010101000,
  0b00000000001010101010101010101000,
  0b00000000101010101010101010101010,
  0b00000000101000101010101010101010,
  0b00000000101010101000001010101010,
  0b00000000001010100000000010101000,
  0b00000000001010000000000000101000,
  0b00000000000010000011110000100000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000011110000000000,
  0b00000000000000001111111100000000,
  0b00000000000000001111111100000000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  //_F_E_D_C_B_A_9_8_7_6_5_4_3_2_1_0
  0b00000000000000000000000000000000,
  0b00000000000001010101010101010101,
  0b00000000000101001101010101010101,
  0b00000000010101000001010101000000,
  0b00000000010101010101010101010101,
  0b00000000010101010101010101010101,
  0b00000000010101010101010101000000,
  0b00000000010101001101010101010101,
  0b00000000010101000001010101010101,
  0b00000000010101010101010101000000,
  0b00000000000101010101010101010101,
  0b00000000000001010101010101010101,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000011110000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000,
  0b00000000000000000000000000000000
};

CRGB pac_palette(byte color) {
  switch(color) {
    case 0b01: return CRGB(255,0,0);
    case 0b10: return CRGB(255,255,0);
    case 0b11: return CRGB(255,255,255);
    default:   return CRGB(0,0,0);
  }
}

void loop_Pacman(uint32_t ms) {
  int idx = (ms>>3) & 0x003F;
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = pac_palette((pacman[idx] >> ((NUM_LEDS - 1 - i)*2)) & 0x0003);
  }  
}

void loop_Red(uint32_t ms) {
  int r = random(2*NUM_LEDS);
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(0, 255, 255 * (r != i));
  }
  delay(20);
}

void loop_Pong(uint32_t ms) {
  byte col = ((ms >> 5) & 0x000000FF);
  byte pos = ((ms >> 4) & 0x00000FFF) % NUM_LEDS;
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(col, 255, 255*(pos==i));
  }
}

void loop_RGBBlink(uint32_t ms) { 
  int col = (((ms >> 1) & 0x0000000F) / 5) * 85;
  for( int i = 0; i < NUM_LEDS; i++ ) {
    
    leds[i] = CHSV(col,255,255);

  }
  
}

void loop_WhiteBlink(uint32_t ms) { 
  for( int i = 0; i < NUM_LEDS; i++ ) {
    
    leds[i] = CHSV(60,0,160 * ((ms >> 6) % 2));

  }
  
}

void loop_Rainbow2(uint32_t ms) { 
  Clear();
  int col = (ms>>2);
  for( int i = 0; i < NUM_LEDS/2; i++ ) {
    
    leds[i << 1] = CHSV(col + i*(256/NUM_LEDS),255,255);
    leds[(i << 1)+1] = CHSV(col + (NUM_LEDS - i)*(256/NUM_LEDS),255,255);

  }
  
}

void loop_Rainbow(uint32_t ms) { 
  Clear();
  int col = (ms >> 1);
  for( int i = 0; i < NUM_LEDS/2; i++ ) {
    
    leds[i] = CHSV(col + i*(128/NUM_LEDS),255,255);
    leds[NUM_LEDS - 1 - i] = CHSV(col + i*(128/NUM_LEDS),255,255);

  }
  
}

void loop_Rainbow_Inc(uint32_t ms) { 
  Clear();
  int col = (ms >> 6) % NUM_LEDS;
  for( int i = 0; i <= col; i++ ) {
    
    leds[i] = CHSV(col + i*(255/NUM_LEDS),255,255);

  }
  
}

void loop_RGB_sawtooth(uint32_t ms) { 
  Clear();
  int count = (ms >> 3) % (NUM_LEDS/2);
  int col = ((ms >> 3) / (NUM_LEDS/2)) % 6;
  for( int i = 0; i <= count; i++ ) {
    
    leds[i] = CHSV(col*42,255,255);
    leds[NUM_LEDS - 1 - i] = CHSV(col*42,255,255);

  }
  
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
  if( xOffset > 4 ) { Clear(); return; }
  for ( byte y = 0; y < 7; y++) {
      bool on = ((sprite[xOffset] >> (6 - y)) & 0x01);
      if (on) {
        leds[ NUM_LEDS - 6 - y ]  = CHSV( 170, 255, 255 );
      }
  }
}

void DrawText(char *text, int x) {
  DrawSprite(Char(Font5x7, text[x/6]), x%6);
}

//uint32_t textcounter = 0;
void loop_Text(uint32_t ms) {
  Clear();
  DrawText("HALLO!", (ms >> 4) % 36);
//  textcounter++;
}

boolean check_button() {
  boolean ret = false;
  if( digitalRead(BUTTON_PIN) == LOW ) {
    if( millis() - last_button > 50 ) {
      ret = true;
    }
    last_button = millis();
  }
  return ret;
}

