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

int oldx = 0;
byte tcol = 1;

#define MODE_COUNT 12
#define USER_MODE_COUNT 13

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
      mode = (s >> 4) % MODE_COUNT;
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
      loop_Stars(ms);
      base_Color(CRGB(255,255,255));
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
    case 11:
      Clear();
      base_RGB(ms);
      break;
    default:
      loop_RainbowSpiral(ms);
      base_Color(CRGB(0,0,255));
      break;
  }
  FastLED.show();
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

CRGB rainbowpalette(byte c) {
  c = c % 6;
  return CRGB(255*(c>2), 255*(((c+5)%6)<3), 255*(((c+1)%6)<3) );
}

void loop_RainbowSpiral(uint32_t ms) {
  for( byte i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = rainbowpalette((byte)(ms>>5) + ((i + ((byte)(ms>>4) & 0x01))>>1));
  }
}

void loop_Stars(uint32_t ms) {
  int r = random(NUM_LEDS);
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(random(255), 255, 255 * (r == i || r-(NUM_LEDS/2) == i));
  }
  delay(20);
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

/*
void loop_Pong(uint32_t ms) {
  byte col = ((ms >> 5) & 0x000000FF);
  byte pos = ((ms >> 4) & 0x00000FFF) % NUM_LEDS;
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(col, 255, 255*(pos==i));
  }
}
*/
void loop_Pong(uint32_t ms) {
  byte col = ((ms >> 5) & 0x000000FF);
  byte pos1 = (NUM_LEDS/2) + (cos(((ms >> 1) & 0x0000FFFF) / (8*PI)) * (NUM_LEDS/2));
  byte pos2 = (NUM_LEDS/2) + (sin(((ms >> 1) & 0x0000FFFF) / (5*PI)) * (NUM_LEDS/2));
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(col, 255, 255*(pos1==i || pos2==i));
  }
}

void loop_FunnyStars(uint32_t ms) {
  byte col = ((ms >> 5) & 0x000000FF);
  byte pos = cos(ldexp((double)(ms & 0x0000FFFF), 16)) * NUM_LEDS;
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
  byte lastled = ((ms >> 4) & 0x0FFF) % NUM_LEDS;
  byte offset = (((ms >> 6) & 0x00FF) % NUM_LEDS) * (255/NUM_LEDS);
  for( int i = 0; i <= lastled; i++ ) {
    
    leds[NUM_LEDS - 1 - i] = CHSV(offset + lastled + i*(255/NUM_LEDS),255,255);

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

static uint16_t Font5x12[] = {
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,
  0b0000000000000000,

  0b0000001111111100,
  0b0000010000011010,
  0b0000100001100001,
  0b0000010110000010,
  0b0000001111111100,
                    
  0b0000000100000001,
  0b0000011000000001,
  0b0000111111111111,
  0b0000000000000001,
  0b0000000000000001,
                    
  0b0000010000000011,
  0b0000100000001101,
  0b0000100000110001,
  0b0000010001000001,
  0b0000001110000001,
                    
  0b0000010000000010,
  0b0000100000000001,
  0b0000100001000001,
  0b0000010010100010,
  0b0000001100011100,
                    
  0b0000000001000000,
  0b0000000111000000,
  0b0000011001000000,
  0b0000100001000000,
  0b0000111111111111,
                    
  0b0000111111000010,
  0b0000100001000001,
  0b0000100001000001,
  0b0000100000100010,
  0b0000100000011100,
                    
  0b0000000111111100,
  0b0000001001000010,
  0b0000010001000001,
  0b0000100000100010,
  0b0000100000011100,
                    
  0b0000100000000000,
  0b0000100000001111,
  0b0000100001110000,
  0b0000100110000000,
  0b0000111000000000,
                    
  0b0000001100011100,
  0b0000010010100010,
  0b0000100001000001,
  0b0000010010100010,
  0b0000001100011100,
                    
  0b0000001100000000,
  0b0000010010000000,
  0b0000100001000011,
  0b0000010001001100,
  0b0000001111110000,
                    
  0b0000000111111111,
  0b0000011001000000,
  0b0000100001000000,
  0b0000011001000000,
  0b0000000111111111,
                    
  0b0000111111111111,
  0b0000100001000001,
  0b0000100001000001,
  0b0000010010100010,
  0b0000001100011100,
                    
  0b0000001111111100,
  0b0000010000000010,
  0b0000100000000001,
  0b0000100000000001,
  0b0000010000000010,
                    
  0b0000111111111111,
  0b0000100000000001,
  0b0000100000000001,
  0b0000010000000010,
  0b0000001111111100,
                    
  0b0000111111111111,
  0b0000100001000001,
  0b0000100001000001,
  0b0000100001000001,
  0b0000100000000001,
                    
  0b0000111111111111,
  0b0000100001000000,
  0b0000100001000000,
  0b0000100001000000,
  0b0000100000000000,
                    
  0b0000001111111100,
  0b0000010000000010,
  0b0000100000000001,
  0b0000100001000001,
  0b0000010001111110,
                    
  0b0000111111111111,
  0b0000000001000000,
  0b0000000001000000,
  0b0000000001000000,
  0b0000111111111111,
                    
  0b0000000000000000,
  0b0000100000000001,
  0b0000111111111111,
  0b0000100000000001,
  0b0000000000000000,
                    
  0b0000100000000010,
  0b0000100000000001,
  0b0000100000000001,
  0b0000100000000010,
  0b0000111111111100,
                    
  0b0000111111111111,
  0b0000000010100000,
  0b0000001100011000,
  0b0000010000000110,
  0b0000100000000001,
                    
  0b0000111111111111,
  0b0000000000000001,
  0b0000000000000001,
  0b0000000000000001,
  0b0000000000000001,
                    
  0b0000111111111111,
  0b0000011000000000,
  0b0000000111000000,
  0b0000011000000000,
  0b0000111111111111,
                    
  0b0000111111111111,
  0b0000001100000000,
  0b0000000011110000,
  0b0000000000001100,
  0b0000111111111111,
                    
  0b0000001111111100,
  0b0000010000000010,
  0b0000100000000001,
  0b0000010000000010,
  0b0000001111111100,
                    
  0b0000111111111111,
  0b0000100001000000,
  0b0000100001000000,
  0b0000010010000000,
  0b0000001100000000,
                    
  0b0000001111111100,
  0b0000010000000010,
  0b0000100000001101,
  0b0000010000000010,
  0b0000001111111101,
                    
  0b0000111111111111,
  0b0000100001000000,
  0b0000100001110000,
  0b0000010010001100,
  0b0000001100000011,
                    
  0b0000001100000001,
  0b0000010010000001,
  0b0000100001000001,
  0b0000100000100010,
  0b0000100000011100,
                    
  0b0000100000000000,
  0b0000100000000000,
  0b0000111111111111,
  0b0000100000000000,
  0b0000100000000000,
                    
  0b0000111111111100,
  0b0000000000000010,
  0b0000000000000001,
  0b0000000000000010,
  0b0000111111111100,
                    
  0b0000111111100000,
  0b0000000000011100,
  0b0000000000000011,
  0b0000000000011100,
  0b0000111111100000,
                    
  0b0000111111111100,
  0b0000000000000011,
  0b0000000001111100,
  0b0000000000000011,
  0b0000111111111100,
                    
  0b0000111000000111,
  0b0000000110011000,
  0b0000000001100000,
  0b0000000110011000,
  0b0000111000000111,
                    
  0b0000111000000000,
  0b0000000110000000,
  0b0000000001111111,
  0b0000000110000000,
  0b0000111000000000,
                    
  0b0000100000000111,
  0b0000100000011001,
  0b0000100011100001,
  0b0000101100000001,
  0b0000110000000001
};

uint16_t* Char(uint16_t* font, char c) {
  if( c > 0x2F && c < 0x3A ) {             //numbers
    c -= 0x2F;
  } else if( c > 0x40 && c < 0x5B ) {      //big letters
    c -= 0x36;
  } else if( c > 0x60 && c < 0x7B ) {      //small letters -> mapped to big letters
    c -= 0x56;
  } else {                                 //any unsupported character
    c = 0;
  }
  return &font[c * 5];
}

void DrawSprite(uint16_t* sprite, int xOffset, CRGB col) {
  if( xOffset > 4 ) { Clear(); return; }
  for ( byte y = 0; y < 12; y++) {
      if ((sprite[xOffset] >> (11 - y)) & 0x01) {
        leds[ y ]  = col;
      }
  }
}

void DrawText(char *text, int x) {
  if(oldx > x) {
    tcol = (tcol + 1) & 0x07;
    if( tcol == 0 ) tcol = 1;
  }
  oldx = x;
  DrawSprite(Char(Font5x12, text[x/6]), x%6, CRGB(255 * (tcol & 0x01), 255 * ((tcol >> 1) & 0x01), 255 * ((tcol >> 2) & 0x01)));
}

void loop_Text(uint32_t ms) {
  Clear();
  DrawText("PEACE ", ((ms >> 3) & 0x00000FFF) % 36);
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


