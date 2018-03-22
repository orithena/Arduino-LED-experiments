#define FASTLED_ALLOW_INTERRUPTS 0
//#define DEBUG
//#define DEBUG_TAP
//#define DEBUG_PIR
//#define DEBUG_SLEEP

#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

#define DATA_PIN    D4
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    15
CRGB leds[NUM_LEDS];
CRGB real_leds[NUM_LEDS];

#define BRIGHTNESS          255
#define FRAMES_PER_SECOND   100
#define BALLS 12
#define TIMEOUT 600000

#ifdef DEBUG_SLEEP
#define TIMEOUT 6000
#endif

// List of patterns to cycle through.  Each is defined as a separate function below.
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
uint8_t gCurrentPatternNumber = 0; // Index number of which pattern is current
uint8_t brightness = BRIGHTNESS;

double balls[BALLS][2];
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void paintball(double pos, uint8_t width, uint8_t hue) 
{
  for( int i = pos-width; i <= pos+width; i++ ) {
    if( i >= 0 && i < NUM_LEDS ) {
      uint8_t val = max(0, cos(((((double)i)-pos)*(PI*0.5)) / ((double)width)) * 255);
#ifdef DEBUG
      Serial.print(i);
      Serial.print("\t");
      Serial.print(val);
      Serial.print("\t");
#endif
      CRGB new_val = CHSV(hue, 255, val);
      CRGB old_val = leds[i];
      for( uint8_t c = 0; c < 3; c++ ) {
        if( new_val[c] > old_val[c] ) {
          leds[i][c] = new_val[c];
        }
#ifdef DEBUG
        Serial.print(new_val[c]>>4, HEX);
        Serial.print(new_val[c]&0x0F, HEX);
        Serial.print(" ");
#endif
      }
#ifdef DEBUG
      Serial.print("  ");
#endif
    }
  }
#ifdef DEBUG
  Serial.println();
#endif
}


#define FREQUENCY    80                  // valid 80, 160
#include "ESP8266WiFi.h"
extern "C" {
#include "user_interface.h"
}
  
void setup() {
  //delay(3000); // 3 second delay for recovery
  
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(2);                                // give RF section time to shutdown
  system_update_cpu_freq(FREQUENCY);

  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(real_leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
  gHue = 0;
  gCurrentPatternNumber = 0;

  init_balls();

  pinMode(A0, INPUT);
  
#if defined(DEBUG) || defined(DEBUG_PIR) || defined(DEBUG_SLEEP) || defined(DEBUG_TAP)
  Serial.begin(230400);

  delay(2000);
/*  for( uint16_t i = 0; i < 5120; i++ ) {
    Serial.printf("%8d", cos16((i-1024)<<3));
    if(i%10 == 0) {
      Serial.println();
    }
  }
  */
#endif

}

typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { pulse, rainbow, pingpong, pulse, confetti, sinelon, juggle };

int32_t old = 0;
int32_t delta = 0;
float mean = 20.0;
float longmean = 20.0;
uint32_t startpress = 0;

uint32_t analog_in() {
  uint32_t ms = millis();
  int cur = analogRead(A0);
  delta = cur - old;
  mean += ((cur - mean) / 32.0);
  longmean += ((cur - longmean) / 2048.0);
  if( longmean > mean ) {
    longmean = mean;
  }
  if( mean < longmean * 2.0 ) {
    startpress = ms;
  }
  #ifdef DEBUG_TAP
  if( delta != 0 ) {
    Serial.print(micros());
    Serial.print("\t");
    Serial.print(mean);
    Serial.print("\t");
    Serial.print(longmean);
    Serial.print("\t");
    Serial.print(ms - startpress);
    Serial.print("\t");
    Serial.print(cur);
    Serial.print("\t");
    Serial.print(delta);
    Serial.print("\t");
    for( int i = 0; i < cur / 8; i++) {
      Serial.print("-");
    }
    Serial.println();
  }
  #endif
  old = cur;
  return ms - startpress;
}

uint32_t night_start = 0;

void loop()
{
  uint32_t hold = analog_in();
  if( hold > 255 ) {
    night_start = 0;
  }
  if( hold > 2048 ) {
    night_start = millis();
  }
  if( night_start > 0 && millis() - night_start > (1000 * 60 * 60 * 8) ) {
    night_start = 0;
  }
  
  wdt_reset();
  // Call the current pattern function once, updating the 'leds' array
  if( night_start > 0 ) {
    nightlight();
  } else {
    gPatterns[gCurrentPatternNumber]();
  }

  for( int i = 0; i < NUM_LEDS; i++ ) {
    real_leds[i].r += (leds[i].r - real_leds[i].r) * 0.0625;
    real_leds[i].g += (leds[i].g - real_leds[i].g) * 0.0625;
    real_leds[i].b += (leds[i].b - real_leds[i].b) * 0.0625;
  }

  // send the 'leds' array out to the actual LED strip
  FastLED.show();  
  // insert a delay to keep the framerate modest
  FastLED.delay(1000/FRAMES_PER_SECOND); 

  // do some periodic updates
  EVERY_N_MILLISECONDS( 80 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  EVERY_N_SECONDS( 90 ) { nextPattern(); } // change patterns periodically
}

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
  init_balls();
}

uint8_t breathing(uint32_t ms)
{
  uint16_t t = ms & 0b000111111111111;
  if( t < 512 ) {
    return cubicwave8(t >> 2);
  } else {
    return cubicwave8(128 + ((t-512) >> 5 ));
  }
}

uint8_t breathing2(uint32_t ms)
{
  uint16_t t = ms % 8192;
  uint8_t ret = 0;
  if( t < 1536 ) {
    ret = cubicwave8((t+512) >> 4);
  } else {
    ret = (( cos16((t-1536)<<2) + 32768 ) >> 8 );
  }
  return ret;
}

void pulse()
{
  for( uint8_t led = 0; led < NUM_LEDS; led++ ) {
    leds[led] = CHSV( 170 + (cubicwave8(gHue)>>4), 255, 192+(cubicwave8(gHue)>>2) );
  }
  for( uint8_t led = (NUM_LEDS/2.5); led < ((NUM_LEDS*3)/5); led++ ) {
    leds[led] = CHSV(0, 255, breathing2(millis()));
  }
}

void nightlight()
{
  for( uint8_t led = 0; led < NUM_LEDS; led++ ) {
    leds[led] = CHSV( 0, 255, 128);
  }
  for( uint8_t led = (NUM_LEDS/2.5); led < ((NUM_LEDS*3)/5); led++ ) {
    leds[led] = CHSV( 255-(breathing2(millis())/32.0), 255, 64+(breathing2(millis())*0.25));
  }
}

#define FADE 32
#define SPEED 0.25

void init_balls() 
{
  for( uint8_t ball = 0; ball < BALLS; ball++) {
    balls[ball][0] = random(NUM_LEDS);
    balls[ball][1] = (random(NUM_LEDS) / 50.0) - (NUM_LEDS/100.0);
  }
}

void pingpong() 
{
  fadeToBlackBy( leds, NUM_LEDS, FADE);
  for( uint8_t ball = 0; ball < BALLS; ball++ ) {
    balls[ball][0] = balls[ball][0] + (balls[ball][1] * SPEED);
    if( (balls[ball][0] < 0) || (balls[ball][0] > NUM_LEDS) ) {
      balls[ball][1] = balls[ball][1] * -1.0;
    }
    paintball(balls[ball][0], 3, (uint8_t)(gHue + (256/BALLS)*ball));
  }
#ifdef DEBUG
  Serial.println();
#endif
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
  addGlitter(FADE*3);
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
  EVERY_N_MILLISECONDS(20) { fadeToBlackBy( leds, NUM_LEDS, 2); }
  for( int i = 0; i < (random8(2)); i++ ) {
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV( gHue + random8(128), 255, random8(128));
  }
}

void sinelon()
{
  // a colored dot sweeping back and forth, with fading trails
  EVERY_N_MILLISECONDS(30) { fadeToBlackBy( leds, NUM_LEDS, 1); }
  int pos = beatsin16(5,0,NUM_LEDS);
  leds[pos] = CHSV( gHue, 255, 255);
}

void bpm()
{
  // colored stripes pulsing at a defined Beats-Per-Minute (BPM)
  uint8_t BeatsPerMinute = 17;
  CRGBPalette16 palette = RainbowColors_p;
  uint8_t beat = beatsin8( BeatsPerMinute, 64, 255);
  for( int i = 0; i < NUM_LEDS; i++) { //9948
    leds[i] = ColorFromPalette(palette, beat-gHue+(i*2), gHue+(i*10));
  }
}

void juggle() {
  // four colored dots, weaving in and out of sync with each other
  fadeToBlackBy( leds, NUM_LEDS, 4+(0.5*FADE));
  byte dothue = gHue;
  for( int i = 0; i < 4; i++) {
    paintball(beatsin16(i+1,0,NUM_LEDS), 2, dothue);
    dothue += 32+(64*SPEED);
  }
}

#if defined(DEBUG) || defined(DEBUG_PIR) || defined(DEBUG_SLEEP)
void printbar(int n, int t, char c1, char c2) {
  for( int i = 0; i < n; i++ ) {
    if( i <= t ) {
      Serial.print(c1);
    } else {
      Serial.print(c2);
    }
  }
  Serial.println();
}
#endif
