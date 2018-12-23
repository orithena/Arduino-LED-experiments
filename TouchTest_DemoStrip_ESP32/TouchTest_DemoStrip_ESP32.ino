#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif
#include "WiFi.h"

#define DATA_PIN    5
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    150
CRGB leds[NUM_LEDS];

#define BRIGHTNESS         255
#define MAX_POWER_MW       300
#define FRAMES_PER_SECOND  60

#include "NoiselessTouchESP32.h"

NoiselessTouchESP32 touch(T5, 8, 3);

void setup() {
    
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  //FastLED.setMaxPowerInVoltsAndMilliamps(5, MAX_POWER_MW);
    
  Serial.begin(230400);
  Serial.println("\n\nHello!\n");
}

#define member_size(type, member) sizeof(((type *)0)->member)

void test_touchvalue() {
  Touchdata d1 = {
    pin : 0,
    history : {64, 64, 64, 64, 64, 64, 64, 255, 64, 64, 64, 64, 64, 64, 64, 64},
    hist_len : 16,
    hist_cur : 0,
    hysteresis : 5,
    last : 0,
    last_event : 1,
    last_event_ms : 0
  };
  Serial.printf("Result d1: %d\n", touch_value(&d1)); 
  Touchdata d2 = {
    pin : 0,
    history : {62, 0, 62, 62, 62, 62, 62, 255, 62, 64, 64, 64, 64, 64, 64, 64},
    hist_len : 16,
    hist_cur : 0,
    hysteresis : 5,
    last : 0,
    last_event : 1,
    last_event_ms : 0
  };
  Serial.printf("Result d2: %d\n", touch_value(&d2)); 
  Touchdata d3 = {
    pin : 0,
    history : {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 255, 0},
    hist_len : 16,
    hist_cur : 0,
    hysteresis : 4,
    last : 0,
    last_event : 1,
    last_event_ms : 0
  };
  Serial.printf("Result d3: %d\n", touch_value(&d3)); 
  Touchdata d4 = {
    pin : 0,
    history : {12, 32, 15, 30, 23, 19, 32, 23, 29, 14, 26, 27, 31, 13, 255, 28},
    hist_len : 16,
    hist_cur : 0,
    hysteresis : 3,
    last : 0,
    last_event : 1,
    last_event_ms : 0
  };
  Serial.printf("Result d4: %d\n", touch_value(&d4)); 
  Touchdata d5 = {
    pin : 0,
    history : {12, 32, 15, 30, 23, 19, 32, 19, 17, 24, 26, 18, 31, 23, 255, 28},
    hist_len : 16,
    hist_cur : 0,
    hysteresis : 2,
    last : 0,
    last_event : 1,
    last_event_ms : 0
  };
  Serial.printf("Result d5: %d\n", touch_value(&d5)); 
}

int touch_value(struct Touchdata *touch) {
  int minimum = touch->history[0];
  int maximum = touch->history[0];
  int mean = 0;
  uint32_t sum = 0;
  for( int i = 0; i < touch->hist_len; i++) {
    minimum = _min(minimum, touch->history[i]);
    maximum = _max(maximum, touch->history[i]);
    sum += touch->history[i];
  }
  mean = sum / touch->hist_len;
  //Serial.printf("mean: %d\n", mean);
  while( mean - minimum > touch->hysteresis || maximum - mean > touch->hysteresis ) {
//  while( abs(maximum - minimum) > touch->hysteresis*2 ) {
    //Serial.printf("abs(%d - %d) = %d\n", maximum, minimum, abs(maximum - minimum));
    int ldelta = abs(mean - minimum);
    int udelta = abs(maximum - mean);
    int lbound = minimum + (ldelta/2);
    int ubound = maximum - (udelta/2);
    if( ldelta < udelta ) {
      lbound = minimum;
    } else {
      ubound = maximum;
    }
    //Serial.printf("Min: %d, Lower: %d, Mean: %d, Upper: %d, Max: %d\n", minimum, lbound, mean, ubound, maximum);
    sum = 0;
    int count = 0;
    minimum = ubound;
    maximum = lbound;
    for( int i = 0; i < touch->hist_len; i++) {
      if( lbound <= touch->history[i] && touch->history[i] <= ubound ) {
        minimum = _min(minimum, touch->history[i]);
        maximum = _max(maximum, touch->history[i]);
        sum += touch->history[i];
        count++;
      }
    }
    if(count == 0) {
      return mean;
    }
    mean = sum/count;
  }
  return mean;
}

int tr(uint8_t pin) {
  int r = touchRead(pin);
//  Serial.printf("pin %d: %d", pin, r);
  return r;
}

struct Touchdata touch_init(uint8_t pin, int history_length, int hysteresis) {
  Touchdata touch = {
    pin : pin,
    history : { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    hist_len : _min(member_size(Touchdata, history), history_length),
    hist_cur : 0,
    hysteresis : hysteresis,
    last : 0
  };
  for( touch.hist_cur = 0; touch.hist_cur < touch.hist_len; touch.hist_cur++ ) {
    touch.history[touch.hist_cur] = tr(touch.pin);
  }
  touch.last = touch_value(&touch);  
  return touch;
}

int touch_hysterese(struct Touchdata *touch) {
  touch->hist_cur = (touch->hist_cur + 1) % touch->hist_len;
  touch->history[touch->hist_cur] = touchRead(touch->pin);
  int val = touch_value(touch);
  //Serial.printf("last: %d, hysteresis: %d, new value: %d\n", touch->last, touch->hysteresis, val);
  if( val < touch->last - touch->hysteresis || touch->last + touch->hysteresis < val ) {
    touch->last = val;
  }
  return touch->last;
}

int touched(struct Touchdata *touch) {
  int last = touch->last;
  int val = touch_hysterese(touch);
  if( val > last ) {
    if( millis()-touch->last_event_ms < 5000 && touch->last_event == -1 ) {
      return 0;
    }
    touch->last_event = -1;
    touch->last_event_ms = millis();
    return -1;
  }
  if( val < last ) {
    if( millis()-touch->last_event_ms < 5000 && touch->last_event == 1 ) {
      return 0;
    }
    touch->last_event = 1;
    touch->last_event_ms = millis();
    return 1;
  }
  return 0;
}

// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
//SimplePatternList gPatterns = { juggle, confetti, rainbow, rainbowWithGlitter, bpm };
//SimplePatternList gPatterns = { sinefield };
SimplePatternList gPatterns = { touch2 };

uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
const uint8_t kMatrixHeight = 14;
const uint8_t kMatrixWidth = 14;

  
void loop()
{
  EVERY_N_MILLISECONDS( 1000/FRAMES_PER_SECOND ) {
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
  
    // send the 'leds' array out to the actual LED strip
    FastLED.show();  
  }
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 20 ) { nextPattern(); } // change patterns periodically
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

void sinefield() {
  double step = (millis() >> 6) & 0x003FFFFF; 
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

void touch1() 
{
  double dnum = touchRead(T5);
  Serial.println(dnum);
  int32_t num = dnum;
  num = _max(1, _min(num, NUM_LEDS));
  fill_rainbow( leds, num, gHue, 7);
  fill_solid( &leds[num], NUM_LEDS-num, CRGB(0,0,0));
}

void touch2() 
{
  int t = touch.changed();
  int num = touch.last_value();
  if(t < 0) {
    Serial.printf("%16d: touch far:  %d\n", millis(), num);
  }
  if(t > 0) {
    Serial.printf("%16d: touch near: %d\n", millis(), num);
  }
  fill_rainbow( leds, num, gHue, 7);
  fill_solid( &leds[num], NUM_LEDS-num, CRGB(0,0,0));
}

void touch3() 
{
  int num = touch.read_with_hysteresis();
  Serial.printf("Touch result: %d\n", num);
  fill_rainbow( leds, num, gHue, 7);
  fill_solid( &leds[num], NUM_LEDS-num, CRGB(0,0,0));
}

void touch4() 
{
  bool t = touch.touched();
  int num = touch.last_value();
  if(t) {
    Serial.printf("%16d: touched!  %d\n", millis(), num);
  }
  fill_rainbow( leds, num, gHue, 7);
  fill_solid( &leds[num], NUM_LEDS-num, CRGB(0,0,0));
}

void touch5() 
{
  bool t = touch.touching();
  int num = touch.last_value();
  if(t) {
    Serial.printf("%16d: touching!  %d\n", millis(), num);
  }
  else {
    Serial.printf("%16d:            %d\n", millis(), num);
  }
  fill_rainbow( leds, num, gHue, 7);
  fill_solid( &leds[num], NUM_LEDS-num, CRGB(0,0,0));
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

