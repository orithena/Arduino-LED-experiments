#include <FastLED.h>

#define LED_PIN  6

//#define DEBUG
#define COLOR_ORDER RGB
#define CHIPSET     WS2811

#define BRIGHTNESS 255

// Params for width and height
const uint8_t kMatrixWidth = 10;
const uint8_t kMatrixHeight = 7;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if ( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if ( kMatrixSerpentineLayout == true) {
    if ( !(y & 0x01)) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } 
    else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* leds( leds_plus_safety_pixel + 1);


byte buf[ 4 ][ kMatrixWidth ][ kMatrixHeight ];
byte bufi = 0;

int currentcolor = 0;
int currentlyalive = 0;
int iterations = 0;
uint32_t nextrun = 0;
int fadestep = 0;
int repetitions = 0;

uint16_t XYsafe( int x, int y)
{
  if ( x >= kMatrixWidth) return -1;
  if ( y >= kMatrixHeight) return -1;
  if ( x < 0) return -1;
  if ( y < 0) return -1;
  return XY(x, y);
}


void loop()
{
  uint32_t ms = millis();

  //PaletteTestLoop(ms/10);
  if( ms > nextrun ) {
    if( repetitions > 5 ) {
      GameOfLifeInit();
    } 
    else {
      GameOfLifeLoop();
    }
    nextrun += 384; 
  }
  GameOfLifeFader( 384-(nextrun - ms) );
  FastLED.show();
}

int _mod(int x, int m) {
  int r = x%m;
  return r<0 ? r+m : r;
}

byte valueof(byte b, int x, int y) {
  return buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)];
}

int neighbors(byte b, int x, int y) {
  int count = 0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        int v = valueof(b,xi,yi);
        count += v;
      }
    }
  }
  return count;
}

boolean alive(byte b, int x, int y) {
  return valueof(b,x,y) & 0x01 > 0;
}

void printBoard(int b) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Serial.print(valueof(b, x, y));
      Serial.print(neighbors(b, x, y));
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println();
}

static unsigned char gol[] = {
  //  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
    //  0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0x00
};

void GameOfLifeInit() {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      buf[bufi^0x01][x][y] = 0;
      buf[3][x][y] = 0;
#ifdef DEBUG
      buf[bufi][x][y] = (gol[x] >> y) & 0x01;
      buf[2][x][y] = (gol[x] >> y) & 0x01;
#else
      int r = random(2);
      buf[bufi][x][y] = r;
      buf[2][x][y] = r;
#endif
    }
  }
#ifdef DEBUG
  printBoard(bufi);
#endif
  currentcolor = random(256);
  iterations = 0;
  currentlyalive = 10;
  repetitions = 0;
}

boolean buf_compare(int b1, int b2) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      if( buf[b1][x][y] != buf[b2][x][y] ) {
        return false;
      }
    }
  }
  return true;
}

int generation(int from, int to) {
  int currentlyalive = 0;
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      int n = neighbors(from, x, y);
      if( alive(from,x,y) ) {
        if( n < 2 ) { //underpopulation
          buf[to][x][y] = 0;
        } 
        else if( n > 3 ) { //overpopulation
          buf[to][x][y] = 0;
        } 
        else { //two or three neighbors
          buf[to][x][y] = buf[from][x][y];
          currentlyalive++;
        }
      } 
      else {
        if( n == 3 ) { //reproduction
          buf[to][x][y] = 1;
          currentlyalive++;
        } 
        else {
          buf[to][x][y] = 0;
        }
      }
    }
  }
  return currentlyalive;
}

void GameOfLifeLoop() {
  bufi = bufi ^ 0x01;
  
  // this is the hare. it always computes two generations. It's never displayed.
  int dummy = generation(2,3);
  dummy = generation(3,2);
  
  // this is the tortoise. it always computes one generation. The tortoise is displayed.
  currentlyalive = generation(bufi^0x01, bufi);
  
  // if hare and tortoise meet, the hare just went ahead of the tortoise through a repetition loop.
  if( buf_compare(bufi, 2) ) {
    repetitions++;
  }

  iterations++;

  /*  for( int x = 0; x < kMatrixWidth; x++ ) {
   for( int y = 0; y < kMatrixHeight; y++ ) {
   leds[ XYsafe(x,y) ] = CHSV(currentcolor, 255, 255 * buf[bufi][x][y]);
   }
   }*/
}

void GameOfLifeFader(int cstep) {
  if( cstep <= 192 ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        double from = buf[bufi^0x01][x][y];
        double to = buf[bufi][x][y];
        leds[ XYsafe(x,y) ] = CHSV(currentcolor, 255, 255 * (from + ((to-from) * cstep) / 192) );
      }
    }
  }
}

void PaletteTestLoop(uint32_t ms) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      leds[ XYsafe(x,y) ] = CHSV( (ms+(4*(y+(x*kMatrixHeight))))%256, 255, 255-(3*(x+(y*kMatrixHeight))));
    }
  }
}

void Clear() {
  for ( byte y = 0; y < kMatrixHeight; y++) {
    for ( byte x = 0; x < kMatrixWidth; x++) {
      leds[ XYsafe(x, y)]  = CHSV((16*y)+(47*x), 255, 42);
    }
  }
}


void setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
#ifdef DEBUG
  Serial.begin(115200);
#endif
  randomSeed(analogRead(A5));
}








