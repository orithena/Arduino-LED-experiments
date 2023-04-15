#include <FastLED.h>

FASTLED_USING_NAMESPACE

// FastLED "100-lines-of-code" demo reel, showing just a few 
// of the kinds of animation patterns you can quickly and easily 
// compose using FastLED.  
//
// This example also shows one easy way to define multiple 
// animations patterns and have them automatically rotate.
//
// -Mark Kriegsman, December 2014

#include "ESP8266WiFi.h"

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    D3
#define PULSE_PIN   D6
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    26
#define NUM_SIDE    12
CRGB leds[NUM_LEDS];

uint32_t last_pulse = 0;
uint32_t last_duration = 1000;

#define WHEEL_DIAMETER_MM 205
#define LED_DISTANCE_MM (100/3)

const double sizefactor = (WHEEL_DIAMETER_MM * PI) / LED_DISTANCE_MM;

#define BRIGHTNESS         255
#define FRAMES_PER_SECOND  100

void setup() {
  WiFi.forceSleepBegin();
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  pinMode(PULSE_PIN, INPUT_PULLUP);
  attachInterrupt(PULSE_PIN, rotatepulse, FALLING);

  Serial.begin(230400);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { rainbow, rainbowWithGlitter, confetti, sinelon, juggle, bpm };
SimplePatternList gPatterns = { pulsetrail };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{
  calculatepulse();
  
  EVERY_N_MILLISECONDS(1000/FRAMES_PER_SECOND) {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
    copy_sides();
    glittercontrol();
    taillights();
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
    // insert a delay to keep the framerate modest
  }

  // do some periodic updates
  EVERY_N_MILLISECONDS( 37 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 30 ) { nextPattern(); } // change patterns periodically
  wdt_reset();
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void copy_sides()
{
  for( int a = 0; a < NUM_SIDE; a++ ) {
    CRGB led = leds[a] | leds[NUM_LEDS-a-1];
    leds[NUM_LEDS-a-1] = led;
    leds[a] = led;
  }
}

void taillights() {
  fract8 lightness = (normalize(millis() - last_pulse, 0, last_duration/2) / 2) + 127 ;
  leds[(NUM_LEDS/2)-1] = CHSV(0,255,lightness);
  leds[NUM_LEDS/2] = CHSV(0,255,lightness);
}

void pulsetrail() 
{
  uint32_t ms = millis();
  //fadeToBlackBy(leds, NUM_LEDS, 128);

  double delta_t = (ms - last_pulse);
  uint32_t n = (delta_t/last_duration) * sizefactor;

  if( n < NUM_SIDE ) {
    leds[ n ] = CHSV( (ms>>2)+gHue, 255, 255 );
  }
}

void glittercontrol() {
  double delta_t = (millis() - last_pulse);
  fract8 chanceOfGlitter = mul8by8(normalize(delta_t, 40000, 0), normalize(last_duration, 400, 1023));
  addGlitter(map(chanceOfGlitter, 0, 255, 0, 20));
}

fract8 mul8by8(fract8 a, fract8 b) {
  return ((unsigned int)a*(unsigned int)b) >> 8;
}

fract8 normalize(long x, long from, long to) {
  long f = map(x, from, to, 0L, 255L);
  long clip_0 = max(f, 0L);
  long clip_255 = min(clip_0, 255L);
  return clip_255;
}

void addGlitter( fract8 chanceOfGlitter) 
{
  if( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] |= CRGB::White;
  }
}

volatile uint32_t pulse = 0;
uint32_t oldpulse = 0;
ICACHE_RAM_ATTR void rotatepulse() {
  pulse = millis();
}

void calculatepulse() {
  if(oldpulse != pulse) {
    uint32_t now = pulse;
    unsigned long ld = now - last_pulse;
    if( ld > 120 ) {
      last_duration = _min(ld, 1023);
      gHue += 17;
    }
    last_pulse = now;
    oldpulse = pulse;
  }
}
