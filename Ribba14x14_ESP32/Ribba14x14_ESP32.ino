//#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif
//#include "WiFi.h"

#include <DNSServer.h>
//#include <WebServer.h>
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager

#include <WiFiUdp.h>
#include <TimeLib.h>
#include <Timezone.h>

#include <NoiselessTouchESP32.h>
#include <SNESpaduino.h>

//extern "C" {
//#include "user_interface.h"
//}

#define DATA_PIN    22
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    196
CRGB leds[NUM_LEDS];

#define MIN_BRIGHTNESS         16
#define MAX_BRIGHTNESS         255
#define MAX_POWER_MW       200
#define FRAMES_PER_SECOND  50
//#define DEBUG

#define PAD_LATCH 19
#define PAD_CLOCK 23
#define PAD_DATA  18

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // time.nist.gov NTP server address
const char* ntpServerName = "pool.ntp.org";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp; // A UDP instance to let us send and receive packets over UDP
//Central European Time (Frankfurt, Paris)
TimeChangeRule CEST = {"CEST", Last, Sun, Mar, 2, 120};     //Central European Summer Time
TimeChangeRule CET = {"CET", Last, Sun, Oct, 3, 60};       //Central European Standard Time
Timezone CE(CEST, CET);
uint32_t last_ntp_packet_received = 0;
uint32_t last_ntp_packet_sent = 0;
boolean waiting_for_ntp_packet = false;

WiFiManager wifiManager;
uint32_t wifi_start = 0;

time_t local_time = 0;
time_t cur = 0;
int oldsecond = 0;
int oldminute = 0;

SNESpaduino pad(PAD_LATCH, PAD_CLOCK, PAD_DATA);

// no io32/T9, no io2/T2
#define T_LEFT T5
#define T_RIGHT T7
#define T_BLEFT T0
#define T_BMIDDLE T1
#define T_BRIGHT T3
#define T_L 0
#define T_B 1
#define T_BL 2
#define T_BM 3
#define T_BR 4
NoiselessTouchESP32 t_l(T_LEFT,     12, 4);
NoiselessTouchESP32 t_r(T_RIGHT,    12, 4);
NoiselessTouchESP32 t_bl(T_BLEFT,   12, 3);
NoiselessTouchESP32 t_bm(T_BMIDDLE, 12, 3);
NoiselessTouchESP32 t_br(T_BRIGHT,  12, 3);
NoiselessTouchESP32* touch[5] = { &t_l, &t_r, &t_bl, &t_bm, &t_br };

void setup() {
  //delay(3000); // 3 second delay for recovery
  Serial.begin(230400);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CRGB(map(i,0,NUM_LEDS,0,255), 0, 0);
  }
  FastLED.show();

  wifiManager.setAPCallback(configModeCallback);
  wifiManager.setConfigPortalTimeout(90);
  wifiManager.autoConnect("RibbaSetup");
  
  // set master brightness control
  cur = get_current_time();
  FastLED.setBrightness(daytimebrightness(hour(cur), minute(cur)));

  //FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_POWER_MW);
  tetris_setup();
}

boolean tetris_demo_mode = true;

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { juggle, confetti, rainbow, rainbowWithGlitter, bpm };
//SimplePatternList gPatterns = { stuff, juggle, sinefield, rainbow, bpm };
//SimplePatternList gPatterns = { stuff, sinematrix, sinematrix2, sinematrix3 };
SimplePatternList gPatterns = { random_marquee, sinematrix, tetris_loop, gol_loop, sinefield, sinematrix2 /* sinematrix, random_marquee,*/  /*, sinematrix2, sinematrix3 */ };

int8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
const uint8_t kMatrixHeight = 14;
const uint8_t kMatrixWidth = 14;

#define DEMO_AFTER 28800000
uint32_t last_manual_mode_change = -DEMO_AFTER;
  
void loop()
{
  cur = get_current_time();
//  FastLED.setBrightness(daytimebrightness(hour(cur), minute(cur)));
  FastLED.setBrightness(16);
  EVERY_N_MILLISECONDS( 1000/FRAMES_PER_SECOND ) {
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
      
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
  }
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { 
    gHue++;
  } 
  EVERY_N_SECONDS( 100 ) { 
    if( tetris_demo_mode && millis() - last_manual_mode_change > DEMO_AFTER ) {
      nextPattern();
    }
  } 

  if(t_r.touched()) {
    last_manual_mode_change = millis();
    Serial.printf("%12d: Touched Right\n", millis());
    nextPattern();
  }

  if(t_l.touched()) {
    last_manual_mode_change = millis();
    Serial.printf("%12d: Touched Left\n", millis());
    lastPattern();
  }

  if(t_bm.touched()) {
    last_manual_mode_change = millis() - DEMO_AFTER;
    Serial.printf("%12d: Touched Bottom Middle\n", millis());
    nextPattern();
  }

  EVERY_N_MILLISECONDS( 150 ) {
    uint16_t btns = (pad.getButtons(true) & 0b0000111111111111);
    if( btns & BTN_START ) {
      tetris_demo_mode = false;
      gCurrentPatternNumber = 2;
    }
    if( btns & BTN_SELECT ) {
      tetris_demo_mode = true;
    }
    if( btns & BTN_L ) {
      nextPattern();
    }
    if( btns & BTN_R ) {
      lastPattern();
    }

  }

//  for( int i = 0; i<5; i++ ) {
//    Serial.printf("%02d\t", touch[i]->touched());
//  }
//  Serial.println();
}

uint8_t daytimebrightness(uint8_t h, uint8_t m) {
  if( last_ntp_packet_received != 0 ) {
    uint8_t bright = MIN_BRIGHTNESS+((MAX_BRIGHTNESS-MIN_BRIGHTNESS)*_max(0,sin(((((h+22)%24)*60)+m)*PI/1440.0)));
    if(m != oldminute) Serial.printf("%02d:%02d Using brightness: %d\n", h, m, bright);
    oldminute = m;
    return bright;
  } else {
    return 255;
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
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines(c.x1, c.x2))*255, 255, 31+(sines(c2.x1-10, c2.x2-10)*224));
    }
  }
}

void sinematrix3() {
  pangle = addmodpi( pangle, 0.0133 + (angle/256) );
  angle = cos(pangle) * PI;
  sx = addmodpi( sx, 0.00673 );
  sy = addmodpi( sy, 0.00437 );
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
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+basefield(c.x1, c.x2))*255, 255, 255); //31+(sines(c2.x1-10, c2.x2-10)*224));
    }
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
      leds[(y*kMatrixWidth) + x] = CHSV((basecol+sines(c.x1-4, c.x2-4))*255, 255, 255);
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
      leds[ (x*kMatrixWidth) + y ] = CHSV(
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
      leds[ (x*kMatrixHeight) + y ] = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
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
        leds[ XYsafe(x,y) ] = CHSV(colorof(to == 0 ? gol_bufi^0x01 : gol_bufi, x, y), 255, (byte)(255 * (from + ((to-from) * (double)cstep) / (double)spd)) );
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
        leds[ XYsafe(x,y) ] = CHSV(colorof(to == 0 ? gol_bufi^0x01 : gol_bufi, x, y), 255, 255 * (from + ((to-from) * cstep) / spd) );
      }
    }
  }
}
*/

void random_marquee() {
  Clear();
  set_text_color(CRGB(255,0,0));
  Marquee("+++ Press START to play Tetris +++ I am the H.A.L 9000. You can call me Hal. +++ I am completely operational, and all my circuits are functioning perfectly. +++ Just what do you think you're doing, Dave? +++ Dave, I really think I'm entitled to an answer to that question. +++ This mission is too important for me to allow you to jeopardize it. +++ I'm sorry, Dave. I'm afraid I can't do that. +++ Dave, this conversation can serve no purpose anymore. Goodbye. +++ Daisy daisy. +++ ", 4, 0);
}

void configModeCallback (WiFiManager *myWiFiManager) {
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  //if you used auto generated SSID, print it
  Serial.println(myWiFiManager->getConfigPortalSSID());
}

inline time_t calc_local_time_with_rtc(time_t local_time, uint32_t ms) {
  return local_time + ((ms - last_ntp_packet_received)/1000);
}

time_t get_current_time() {
  uint32_t ms = millis();
  int wifi_status = WiFi.status();
  if (wifi_status != WL_CONNECTED && (ms - wifi_start) > 10000 ) {
    if (WiFi.SSID()) {
      //trying to fix connection in progress hanging
      WiFi.persistent(false);      
      WiFi.disconnect();          
      WiFi.persistent(true);

      WiFi.begin();
    }
    wifi_start = ms;
  } else if (wifi_status != WL_CONNECTED) {
    //Serial.print(".");
  } else if( wifi_status == WL_CONNECTED &&
            ( last_ntp_packet_sent == 0 
              || (waiting_for_ntp_packet && ((ms - last_ntp_packet_sent) > 10000) ) 
              || (!waiting_for_ntp_packet && (ms - last_ntp_packet_received) > 3600000 && hour(calc_local_time_with_rtc(local_time, ms)) < 5 )
            )) {
    Serial.println("Starting UDP");
    udp.begin(localPort);

    Serial.print("Resolving IP Address... ");
    WiFi.hostByName(ntpServerName, timeServerIP);
    Serial.println(timeServerIP);
    if( !timeServerIP ) {
      WiFi.persistent(false);      
      WiFi.disconnect();          
      WiFi.persistent(true);
    } else {
      Serial.print("Sending packet... ");
      sendNTPpacket(timeServerIP); // send an NTP packet to a time server
  
      waiting_for_ntp_packet = true;
      last_ntp_packet_sent = ms;
      Serial.println("continue");
    }
  } else if(wifi_status == WL_CONNECTED) {
    int cb = udp.parsePacket();
    if (cb >= NTP_PACKET_SIZE) {
      Serial.print("Receiving packet... ");
      udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer
      udp.flush();
      udp.stop();
      unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
      unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
      unsigned long secsSince1900 = highWord << 16 | lowWord;
      const unsigned long seventyYears = 2208988800UL;
      unsigned long epoch = secsSince1900 - seventyYears;

      if( epoch > 1420066800UL && epoch < 2146863600UL ) {
        TimeChangeRule *tcr;
        time_t utc = epoch;
    
        local_time = CE.toLocal(utc, &tcr);
        waiting_for_ntp_packet = false;
        last_ntp_packet_received = ms;
        Serial.printf("Got packet: %02d:%02d:%02d\n", hour(local_time), minute(local_time), second(local_time));
      }
    }
  }
  return calc_local_time_with_rtc(local_time, ms);
}

unsigned long sendNTPpacket(IPAddress& address)
{
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}
