//#define FASTLED_ALLOW_INTERRUPTS 0
#include <FastLED.h>
FASTLED_USING_NAMESPACE
#include <LEDMatrix.h>

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif
//#include "WiFi.h"

//extern "C" {
//#include "user_interface.h"
//}

#define DATA_PIN    16
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    256
#define MATRIX_WIDTH 16
#define MATRIX_HEIGHT 16
//CRGB leds(NUM_LEDS);
cLEDMatrix<MATRIX_WIDTH, MATRIX_HEIGHT, HORIZONTAL_ZIGZAG_MATRIX> leds;

#define MIN_BRIGHTNESS         16
#define MAX_BRIGHTNESS         255
#define MAX_POWER_MW       200
#define FRAMES_PER_SECOND  50
//#define DEBUG

const uint8_t kMatrixHeight = 16;
const uint8_t kMatrixWidth = 16;
const bool kMatrixSerpentineLayout = false;

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void setup() {
  //delay(3000); // 3 second delay for recovery
  Serial.begin(230400);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds[0], leds.Size()).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  //FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_POWER_MW);
  FastLED.setBrightness(96);
  tetris_setup();
  for( int i = 0; i < NUM_LEDS; i++) {
    leds(i) = CRGB(0,0,0);
  }
}

void printmatrix() {
  Serial.println();
  for( int i = 0; i<NUM_LEDS; i++) {
    Serial.printf("%4d ", leds(i).r);
  }
  Serial.println();
  for( int i = 0; i<NUM_LEDS; i++) {
    Serial.printf("%4d ", leds(i).g);
  }
  Serial.println();
  for( int i = 0; i<NUM_LEDS; i++) {
    Serial.printf("%4d ", leds(i).b);
  }
  Serial.println();
  for( int i = 0; i<NUM_LEDS; i++) {
    CHSV x = rgb2hsv_approximate(leds(i));
    Serial.printf("%4d ", x.v);
  }
  Serial.println();
}

boolean tetris_demo_mode = true;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { juggle, confetti, rainbow, rainbowWithGlitter, bpm };
//SimplePatternList gPatterns = { stuff, juggle, sinefield, rainbow, bpm };
//SimplePatternList gPatterns = { snakes };
//SimplePatternList gPatterns = { snakes, sinematrix4, sinefield, tetris_loop, sinematrix3, gol_loop, sinematrix };
SimplePatternList gPatterns = { tetris_loop, perlinmatrix, tetris_loop, doublesnakes, sinematrix, gol_loop, snakes, sinematrix4, tetris_loop, sinefield, gol_loop, sinematrix3 };

int8_t gCurrentPatternNumber = 0;  // Index number of which pattern is current
uint8_t gHue[] = { 0, 0, 0, 0 };   // rotating "base color" used by many of the patterns


typedef struct Vector {
  double x1;
  double x2;
} Vector;

typedef struct Matrix {
  double a11;
  double a12;
  double a21;
  double a22;
} Matrix;

#define DEMO_AFTER 360000
uint32_t last_manual_mode_change = -DEMO_AFTER;
  
void loop()
{
  EVERY_N_MILLISECONDS( 1000/FRAMES_PER_SECOND ) {
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
      
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
  }
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { 
    gHue[0]++;
  } 
  EVERY_N_MILLISECONDS( 60 ) { 
    gHue[1]++;
  } 
  EVERY_N_MILLISECONDS( 180 ) { 
    gHue[2]++;
  } 
  EVERY_N_MILLISECONDS( 540 ) { 
    gHue[3]++;
  } 
  EVERY_N_SECONDS( 240 ) { 
    if( tetris_demo_mode && millis() - last_manual_mode_change > DEMO_AFTER ) {
      nextPattern();
    }
  } 
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE(gPatterns);
}

void lastPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber--;
  if( gCurrentPatternNumber < 0 ) gCurrentPatternNumber += ARRAY_SIZE(gPatterns);
}


uint8_t maxlight( CRGB &led ) {
  return led.r > led.g 
            ? (
                led.r > led.b 
                  ? led.r 
                  : led.b
              ) 
            : (
                led.g > led.b 
                  ? led.g 
                  : led.b
              );
}

void fadeBlur() {
  uint8_t blurAmount = beatsin8(2,10,255);
  blur2d( leds[0], kMatrixWidth, kMatrixHeight, blurAmount);
}

struct Matrix multiply(struct Matrix m1, struct Matrix m2) {
  Matrix r = {
      .a11 = m1.a11*m2.a11 + m1.a12*m2.a21,
      .a12 = m1.a11*m2.a12 + m1.a12*m2.a22,
      .a21 = m1.a21*m2.a11 + m1.a22*m2.a21,
      .a22 = m1.a21*m2.a12 + m1.a22*m2.a22
    };
  return r;
};

struct Vector multiply(struct Matrix m, struct Vector v) {
  Vector r = {
      .x1 = (m.a11*v.x1) + (m.a12*v.x2),
      .x2 = (m.a21*v.x1) + (m.a22*v.x2)
    };
  return r;
}

struct Vector add(struct Vector v1, struct Vector v2) {
  Vector r = {
    .x1 = v1.x1 + v2.x2,
    .x2 = v1.x2 + v2.x2
  };
  return r;
}

inline double sines(double x, double y) {
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

inline double basefield(double x, double y) {
  return (cos(x) * sin(y) * cos(sqrt((x*x) + (y*y))));
}

inline double addmod(double x, double mod, double delta) {
  x = x + delta;
  while( x >= mod ) x -= mod;
  while( x <  0.0 ) x += mod;
  return x;
}

inline double addmodpi(double x, double delta) {
  return addmod(x, 2*PI, delta);
}

double fx = 1.0/(kMatrixWidth/PI);
double fy = 1.0/(kMatrixHeight/PI);

double pangle = 0;
double angle = 0;
double sx = 0;
double sy = 0;
double tx = 0;
double ty = 0;
double cx = 0;
double cy = 0;
double rcx = 0;
double rcy = 0;
double angle2 = 0;
double sx2 = 0;
double sy2 = 0;
double tx2 = 0;
double ty2 = 0;
double basecol = 0;

void sinematrix2() {
  pangle = addmodpi( pangle, 0.0133 + (angle/256) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00173 );
  sy = addmodpi( sy, 0.00137 );
  tx = addmodpi( tx, 0.00239 );
  ty = addmodpi( ty, 0.00293 );
  cx = addmodpi( cx, 0.00197 );
  cy = addmodpi( cy, 0.00227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  angle2 = addmodpi( angle2, 0.0029 );
  sx2 = addmodpi( sx2, 0.0041);
  sy2 = addmodpi( sy2, 0.0031);
  tx2 = addmodpi( tx2, 0.0011 );
  ty2 = addmodpi( ty2, 0.0023 );
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/4.0 + 0.25,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/4.0 + 0.25
  };
  Vector translate = {
    .x1 = (-kMatrixWidth/2) + (sin(tx) * kMatrixWidth) + rcx,
    .x2 = (-kMatrixHeight/2) + (sin(ty) * kMatrixHeight) + rcy
  };

  Matrix rotate2 = {
    .a11 = cos(angle2),
    .a12 = -sin(angle2),
    .a21 = sin(angle2),
    .a22 = cos(angle2)
  };
  Matrix zoom2 = {
    .a11 = sin(sx2)/2.0,
    .a12 = 0,
    .a21 = 0,
    .a22 = sin(sy2)/2.0
  };
  Vector translate2 = {
    .x1 = sin(tx2),
    .x2 = sin(ty2)
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x, .x2 = y } ), translate);
      Vector c2 = add(multiply( multiply(zoom2, rotate2), { .x1 = x, .x2 = y } ), translate2);
      leds(x,y ) = CHSV((basecol+sines(c.x1, c.x2))*255, 255, 31+(sines(c2.x1-10, c2.x2-10)*224));
    }
  }
}

void sinematrix3() {
  pangle = addmodpi( pangle, 0.00333 + (angle/1024.0) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
  tx = addmodpi( tx, 0.000239 );
  ty = addmodpi( ty, 0.000293 );
  cx = addmodpi( cx, 0.000197 );
  cy = addmodpi( cy, 0.000227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/4.0 + 0.15,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/4.0 + 0.15
  };
  Vector translate = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      //Vector c2 = add(multiply( multiply(zoom2, rotate2), { .x1 = x, .x2 = y } ), translate2);
      leds(x,y) = CHSV((basecol+basefield(c.x1, c.x2))*255, 255, 255); //31+(sines(c2.x1-10, c2.x2-10)*224));
    }
  }
}

void sinematrix4() {
  pangle = addmodpi( pangle, 0.00233 + (angle/2048.0) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
  tx = addmodpi( tx, 0.000239 );
  ty = addmodpi( ty, 0.000293 );
  cx = addmodpi( cx, 0.000197 );
  cy = addmodpi( cy, 0.000227 );
  rcx = (kMatrixWidth/2) + (sin(cx) * kMatrixWidth);
  rcy = (kMatrixHeight/2) + (sin(cy) * kMatrixHeight);
  angle2 = addmodpi( angle2, 0.00029 );
  sx2 = addmodpi( sx2, 0.0041);
  sy2 = addmodpi( sy2, 0.0031);
  tx2 = addmodpi( tx2, 0.0011 );
  ty2 = addmodpi( ty2, 0.0023 );
  basecol = addmod( basecol, 1.0, 0.007 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/8.0 + 0.05,
    .a12 = 0, //atan(cos(sx2)),
    .a21 = 0, //atan(cos(sy2)),
    .a22 = cos(sy)/8.0 + 0.05
  };
  Vector translate = {
    .x1 = sin(tx) * kMatrixWidth,
    .x2 = sin(ty) * kMatrixHeight
  };

  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(rotate, zoom), { .x1 = x-rcx, .x2 = y-rcy } ), translate);
      double waveheight = basefield(c.x1, c.x2);
      leds(x,y) = CHSV((basecol+(waveheight*1.5))*255, 255, waveheight*384);
//      Serial.print(leds(x,y).r/26);
    }
//    Serial.println();
  }
}

void sinematrix() {
  angle = addmodpi( angle, 0.019 );
  sx = addmodpi( sx, 0.017);
  sy = addmodpi( sy, 0.013);
  tx = addmodpi( tx, 0.023 );
  ty = addmodpi( ty, 0.029 );
  basecol = addmod( basecol, 1.0, 0.01 );
  
  Matrix rotate = {
    .a11 = cos(angle),
    .a12 = -sin(angle),
    .a21 = sin(angle),
    .a22 = cos(angle)
  };
  Matrix zoom = {
    .a11 = sin(sx)/2.0,
    .a12 = 0,
    .a21 = 0,
    .a22 = sin(sy)/2.0
  };
  Vector translate = {
    .x1 = sin(tx),
    .x2 = sin(ty)
  };
  
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Vector c = add(multiply( multiply(zoom, rotate), { .x1 = x, .x2 = y } ), translate);
      leds(x,y) = CHSV((basecol+sines(c.x1-4, c.x2-4))*255, 255, 255);
    }
  }

//  DrawNumberOneFrame(hour(cur), 10, 7);
//  DrawNumberOneFrame(minute(cur), 7, 0);
} 

double calc(double px, double py, double dx, double dy, double ox, double oy) {
  double x = px + (dx*ox);
  double y = py + (dy*oy);
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

double hpx = 0.5, hpy = 0.5, hdx = PI/8, hdy = PI/8;
double spx = 0.5, spy = 0.5, sdx = 0.0, sdy = 0.0;
double vpx = 0.5, vpy = 0.5, vdx = 0.0, vdy = 0.0;

#define SPR 9
#define SPROFF 4
#define HFX 0.007
#define HFY 0.005
#define SFX 0.002
#define SFY 0.003
#define VFX 0.008
#define VFY 0.012

void stuff() {
  //px += (random(10) - 3) / 1000.0;
  //py += (random(10) - 3) / 1000.0;
  hpx += hdx;  hpy += hdy;  hdx += (double)(random(SPR) - SPROFF) * HFX;  hdy += (double)(random(SPR) - SPROFF) * HFY;
  //if( hpx > PI ) hpx -= PI;  if( hpx < -PI ) hpx += PI;  
  if( hdx > PI/4 ) hdx = -PI/4;  if( hdx < -PI/4 ) hdx = PI/4;
  //if( hpy > PI ) hpy -= PI;  if( hpy < -PI ) hpy += PI;  
  if( hdy > PI/4 ) hdy = -PI/4;  if( hdy < -PI/4 ) hdy = PI/4;
  spx += sdx;  spy += sdy;  sdx += (double)(random(SPR) - SPROFF) * SFX;  sdy += (double)(random(SPR) - SPROFF) * SFY;
  //if( spx > PI ) spx = PI;  if( spx < -PI ) spx = -PI;  
  if( sdx > PI/8 ) sdx = -PI/8;  if( sdx < -PI/8 ) sdx = PI/8;
  //if( spy > PI ) spy = PI;  if( spy < -PI ) spy = -PI;  
  if( sdy > PI/8 ) sdy = -PI/8;  if( sdy < -PI/8 ) sdy = PI/8;
  vpx += vdx;  vpy += vdy;  vdx += (double)(random(SPR) - SPROFF) * VFX;  vdy += (double)(random(SPR) - SPROFF) * VFY;
  //if( vpx > PI ) vpx = PI;  if( vpx < -PI ) vpx = -PI;  
  if( vdx > PI/8 ) vdx = -PI/16;  if( vdx < -PI/16 ) vdx = PI/16;
  //if( vpy > PI ) vpy = PI;  if( vpy < -PI ) vpy = -PI;  
  if( vdy > PI/8 ) vdy = -PI/16;  if( vdy < -PI/16 ) vdy = PI/16;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      leds(x,y ) = CHSV(
        calc(hpx,hpy,hdx,hdy,x,y) * 254,
        ((calc(spx,spy,sdx,sdy,x,y) * 0.5) + 0.5) * 254,
        ((calc(vpx,vpy,vdx,vdy,x,y) * 0.75) + 0.25) * 254
      );
    }
  } 
}

void sinefield() {
  float step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    hue = step + (37 * sin( ((y*step)/(kMatrixHeight*PI)) * 0.04 ) );
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      hue += 17 * sin(x/(kMatrixWidth*PI));
      leds(y,x ) = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
}


void rainbow() 
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds[0], NUM_LEDS, gHue[0], 7);
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
    leds(random16(NUM_LEDS) ) += CRGB::White;
  }
}

void confetti() 
{
  // random colored speckles that blink in and fade smoothly
  fadeToBlackBy( leds[0], NUM_LEDS, 2);
  int pos = random16(NUM_LEDS);
  leds(pos) += CHSV( gHue[0] + random8(64), 200, 255);
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  fadeToBlackBy( leds[0], NUM_LEDS, 64);
  int pos = beatsin16( 1, 0, NUM_LEDS-1 );
  leds(pos) += CHSV( gHue[1], 255, 192);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 62;
  CRGBPalette16 palette = PartyColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds(i) = ColorFromPalette(palette, gHue[1]+(i*2), beat-gHue[1]+(i*10));
  }
}

void juggle() {
  // eight colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds[0], NUM_LEDS, 20);
  byte dothue = 0;
  for( int i = 0; i < 8; i++) {
    leds(beatsin16( (accum88)2.0+((accum88)i*0.5), 0, NUM_LEDS-1 )) |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}

#define GOL_ROUNDTIME 768

byte gol_buf[ 4 ][ kMatrixWidth ][ kMatrixHeight ];
byte gol_bufi = 0;

int currentcolor = 0;
int currentlyalive = 0;
int iterations = 0;
uint32_t nextrun = 0;
int fadestep = 0;
int repetitions = 0;

void gol_loop()
{
  uint32_t ms = millis();

  if( ms > nextrun ) {
    if( repetitions > 5 ) {
      GameOfLifeInit();
    } 
    else {
      GameOfLifeLoop();
    }
    nextrun = ms + GOL_ROUNDTIME; 
  }
  GameOfLifeFader( GOL_ROUNDTIME-(nextrun - ms) );

}

int _mod(int x, int m) {
  int r = x%m;
  return r<0 ? r+m : r;
}

byte valueof(byte b, int x, int y) {
  return gol_buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)] > 0 ? 1 : 0;
}

byte colorof(byte b, int x, int y) {
  return gol_buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)];
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

byte neighborcolor(byte b, int x, int y) {
  double cos_sum = 0.0;
  double sin_sum = 0.0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        if( valueof(b,xi,yi) > 0 ) {
          double phi = (((double)(colorof(b,xi,yi)) * PI) / 128.0);
          cos_sum = cos_sum + cos(phi);
          sin_sum = sin_sum + sin(phi);
        }
      }
    }
  }
  double phi_n = atan2(sin_sum, cos_sum);
  while( phi_n < 0.0 ) phi_n += 2*PI;
  byte color = (phi_n * 128.0) / PI;
  if(color == 0) color = 1;
  return color;
}

boolean alive(byte b, int x, int y) {
  return (valueof(b,x,y) & 0x01) > 0;
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

#ifdef DEBUG
static unsigned char gol[] = {
  //  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
    //  0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0x00
};
#endif

void GameOfLifeInit() {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      gol_buf[gol_bufi^0x01][x][y] = 0;
      gol_buf[3][x][y] = 0;
#ifdef DEBUG
      gol_buf[gol_bufi][x][y] = (gol[x] >> y) & 0x01;
      gol_buf[2][x][y] = (gol[x] >> y) & 0x01;
#else
      byte r;
      if( random(2) == 0) {
        r = 0;
      } else {
        r = (random(3)*85 ) + 1;
      }
      gol_buf[gol_bufi][x][y] = r;
      gol_buf[2][x][y] = r;
#endif
    }
  }
#ifdef DEBUG
  printBoard(gol_bufi);
#endif
  currentcolor = random(256);
  iterations = 0;
  currentlyalive = 10;
  repetitions = 0;
}

boolean gol_buf_compare(int b1, int b2) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      if( (gol_buf[b1][x][y] == 0) != (gol_buf[b2][x][y] == 0) ) {
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
          gol_buf[to][x][y] = 0;
        } 
        else if( n > 3 ) { //overpopulation
          gol_buf[to][x][y] = 0;
        } 
        else { //two or three neighbors
          gol_buf[to][x][y] = gol_buf[from][x][y];
          currentlyalive++;
        }
      } 
      else {
        if( n == 3 ) { //reproduction
          gol_buf[to][x][y] = neighborcolor(from, x, y);
          currentlyalive++;
        } 
        else {
          gol_buf[to][x][y] = 0;
        }
      }
    }
  }
  return currentlyalive;
}

void GameOfLifeLoop() {
  gol_bufi = gol_bufi ^ 0x01;
  
  // this is the hare. it always computes two generations. It's never displayed.
  int dummy = generation(2,3);
  dummy = generation(3,2);
  
  // this is the tortoise. it always computes one generation. The tortoise is displayed.
  currentlyalive = generation(gol_bufi^0x01, gol_bufi);
  
  // if hare and tortoise meet, the hare just went ahead of the tortoise through a repetition loop.
  if( gol_buf_compare(gol_bufi, 2) ) {
    repetitions++;
  }

  iterations++;
}

void GameOfLifeFader(int cstep) {
  int spd = GOL_ROUNDTIME / 2;
  if( cstep >= 0 && cstep <= spd ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        byte from = valueof(gol_bufi^0x01, x, y);
        byte to = valueof(gol_bufi, x, y);
        leds(x,y ) = CHSV(colorof(to == 0 ? gol_bufi^0x01 : gol_bufi, x, y), 255, (byte)(255 * (from + ((to-from) * (double)cstep) / (double)spd)) );
      }
    }
  }
}
/*
void GameOfLifeFader(int cstep) {
  int spd = GOL_ROUNDTIME / 2;
  if( cstep <= spd ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        double from = valueof(gol_bufi^0x01, x, y);
        double to = valueof(gol_bufi, x, y);
        leds(x,y ) = CHSV(colorof(to == 0 ? gol_bufi^0x01 : gol_bufi, x, y), 255, 255 * (from + ((to-from) * cstep) / spd) );
      }
    }
  }
}
*/
#define SHOW_TEXT_DELAY 72
#define SHOW_TEXT_COLOR CHSV(170, 255, 255)

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

void DrawSprite(unsigned char* sprite, int length, int xOffset, int yOffset) {
  //  leds(x, y ) = CHSV(0, 0, 255);
  for ( byte y = 0; y < 7; y++) {
    for ( byte x = 0; x < 5; x++) {
      bool on = (sprite[x] >> (6 - y) & 1) * 255;
      if (on) {
        leds(x + xOffset, y + yOffset ) = SHOW_TEXT_COLOR;
        //fadeToBlackBy(&leds(x + xOffset, y + yOffset), 1, 224);
      }
    }
  }
}

void DrawText(char *text, int x, int y) {
  for (int i = 0; i < strlen(text); i++) {
    DrawSprite(Char(Font5x7, text[i]), 5, x + i * 6, y);
  }
}

void DrawTextOneFrame(char *text, int xOffset, int yOffset) {
  DrawText(text, 10 - xOffset, yOffset);
}

void DrawNumberOneFrame(uint32_t number, int xOffset, int yOffset) {
  char buffer[7];
  //itoa(number,buffer,10);
  sprintf(buffer, "%02d", number);
  DrawTextOneFrame(buffer, xOffset, yOffset); 
}

void ShowText(char *text) {
  for( int i = 0; i < (strlen(text)+4)*6; i++ ) {
    Clear();
    DrawTextOneFrame(text, i, 3);
    FastLED.show();
    delay(SHOW_TEXT_DELAY);
  }
}

void Clear() {
  for ( byte y = 0; y < kMatrixHeight; y++) {
    for ( byte x = 0; x < kMatrixWidth; x++) {
      leds(x, y)  = CHSV((16*y)+(47*x), 255, 42);
    }
  }
}

inline boolean between(long l, long x, long u) {
  return (unsigned)(x - l) <= (u - l);
}
