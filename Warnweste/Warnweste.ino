#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif
#include "ESP8266WiFi.h"
extern "C" {
#include "user_interface.h"
}


#define DATA_PIN    D3
#define BTN_PIN     D4
//#define CLK_PIN   4
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define NUM_LEDS    36
CRGB leds[NUM_LEDS];

#define BRIGHTNESS         96
#define FRAMES_PER_SECOND  80

void setup() {
  WiFi.forceSleepBegin();                  // turn off ESP8266 RF
  delay(2);                                // give RF section time to shutdown
  system_update_cpu_freq(80);
  delay(3000); // 3 second delay for recovery

  Serial.begin(230400);

  pinMode(BTN_PIN, INPUT);
  
  // tell FastLED about the LED strip configuration
  FastLED.addLeds<LED_TYPE,DATA_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
  //FastLED.addLeds<LED_TYPE,DATA_PIN,CLK_PIN,COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  // set master brightness control
  FastLED.setBrightness(BRIGHTNESS);
}


// List of patterns to cycle through.  Each is defined as a separate function below.
typedef void (*SimplePatternList[])();
SimplePatternList gPatterns = { juggle, confetti, sinelon, rainbow, rainbowWithGlitter, bpm, blinken, cycling };
//SimplePatternList gPatterns = { rainbow };

uint8_t gCurrentPatternNumber = 7; // Index number of which pattern is current
uint8_t gHue = 0; // rotating "base color" used by many of the patterns
  
void loop()
{

  char morse = morse_decode();
  if( morse != '\0' ) {
    Serial.println(morse);
    switch( morse ) {
      case 'J': gCurrentPatternNumber = 0;
                break; 
      case 'C': gCurrentPatternNumber = 1;
                break; 
      case 'S': gCurrentPatternNumber = 2;
                break; 
      case 'R': gCurrentPatternNumber = 3;
                break;
      case 'G': gCurrentPatternNumber = 4;
                break; 
      case 'B': gCurrentPatternNumber = 6;
                break; 
      case 'I': gCurrentPatternNumber = 7;
                FastLED.setBrightness(96);
                break; 
      case 'O': FastLED.setBrightness(0);
                break;
      case '0': FastLED.setBrightness(0);
                break;
      case '1': FastLED.setBrightness(8);
                break;
      case '2': FastLED.setBrightness(24);
                break;
      case '3': FastLED.setBrightness(48);
                break;
      case '4': FastLED.setBrightness(96);
                break;
      case '5': FastLED.setBrightness(255);
                break;
    }
  }

  EVERY_N_MILLISECONDS(1000/FRAMES_PER_SECOND) {
    // Call the current pattern function once, updating the 'leds' array
    gPatterns[gCurrentPatternNumber]();
    
    // send the 'leds' array out to the actual LED strip
    FastLED.show();
  }
  
  // do some periodic updates
  EVERY_N_MILLISECONDS( 20 ) { gHue++; } // slowly cycle the "base color" through the rainbow
  // EVERY_N_SECONDS( 20 ) { nextPattern(); } // change patterns periodically
  wdt_reset();
}

static const struct {const char letter, *code;} MorseMap[] =
{
  { 'A', ".-" },
  { 'B', "-..." },
  { 'C', "-.-." },
  { 'D', "-.." },
  { 'E', "." },
  { 'F', "..-." },
  { 'G', "--." },
  { 'H', "...." },
  { 'I', ".." },
  { 'J', ".---" },
  { 'K', ".-.-" },
  { 'L', ".-.." },
  { 'M', "--" },
  { 'N', "-." },
  { 'O', "---" },
  { 'P', ".--." },
  { 'Q', "--.-" },
  { 'R', ".-." },
  { 'S', "..." },
  { 'T', "-" },
  { 'U', "..-" },
  { 'V', "...-" },
  { 'W', ".--" },
  { 'X', "-..-" },
  { 'Y', "-.--" },
  { 'Z', "--.." },
  { ' ', "     " }, //Gap between word, seven units 
    
  { '1', ".----" },
  { '2', "..---" },
  { '3', "...--" },
  { '4', "....-" },
  { '5', "....." },
  { '6', "-...." },
  { '7', "--..." },
  { '8', "---.." },
  { '9', "----." },
  { '0', "-----" },
    
  { '.', "·-·-·-" },
  { ',', "--..--" },
  { '?', "..--.." },
  { '!', "-.-.--" },
  { ':', "---..." },
  { ';', "-.-.-." },
  { '(', "-.--." },
  { ')', "-.--.-" },
  { '"', ".-..-." },
  { '@', ".--.-." },
  { '&', ".-..." },
};

#define RING_SIZE 20
#define DEFAULT_THRESHOLD 250
#define MAX_DELAY 1000
int old_button_state = HIGH;
uint32_t hist[RING_SIZE];
uint32_t lastchange = 0;
byte ringstart=0;
byte ringend=0;

inline int diff(byte rstart, byte rend) {
  if( rstart > rend ) {
    return (rend + RING_SIZE) - rstart;
  } else {
    return rend - rstart;
  }
}

inline int32_t min(int32_t a, int32_t b) {
  if( a < b ) {
    return a;
  } else {
    return b;
  }
}
inline int32_t max(int32_t a, int32_t b) {
  return -min(-a, -b);
}
char morse_decode() {
  uint32_t ms = millis();
  int button_state = digitalRead(BTN_PIN);
  if( ms - lastchange < 10 ) {
    return '\0';
  }
  if( button_state != old_button_state ) {
    hist[ringend] = ms - lastchange;
    lastchange = ms;
    ringend = (ringend + 1) % RING_SIZE;
  }
  if( diff(ringstart, ringend) >= 2 ) {
    uint32_t mean = 0;
    int mean_count = 0;
    uint32_t hmin = 0x7FFFFFFF;
    uint32_t hmax = 0;
    int c = ringstart;
    Serial.print("hist: ");
    while( diff(c, ringend) > 0 ) {
      if( hist[c] < MAX_DELAY ) {
        hmin = min(hmin, hist[c]);
        hmax = max(hmax, hist[c]);
      }
      Serial.printf("%4d%c  ", hist[c], c % 2 == 0 ? '.' : '#');
      c = (c+1) % RING_SIZE;
    }
    Serial.printf("%4d, | hmin:%3d hmax:+%3d | ", ms - lastchange, hmin, hmax);
    uint32_t threshold = hmin + ((hmax - hmin) / 2);
    if( hmin > (hmax/2) ) {
      // only short pulses/gaps or only one pulse detected
      threshold = hmax * 2;
    }
    if( diff(ringstart, ringend) <= 2 ) {
      // only one pulse detected
      threshold = DEFAULT_THRESHOLD;
    }
    Serial.printf("T=%4d |", threshold);
    c = ringstart;
    String morsecode = "";
    while( diff(c, ringend) > 0 ) {
      if( c % 2 == 1 ) {
        if( hist[c] > threshold ) {
          morsecode += "-";
        } else {
          morsecode += ".";
        }
        if( diff(c, ringend) == 1 ) {
          Serial.print(morsecode);
          if( ms - lastchange > (threshold*2) ) {
            // ok, that last pause got too long, so we reset the ringbuffer
            ringstart = ringend;
            for( int i = 0; i < sizeof MorseMap / sizeof *MorseMap; ++i ) {
              if( morsecode == MorseMap[i].code ) {
                return MorseMap[i].letter;
              }
            }
          }
        }
      }
      c = (c+1) % RING_SIZE;
    }
    Serial.println();
  }
  old_button_state = button_state;
  return '\0';
}

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

void nextPattern()
{
  // add one to the current pattern number, and wrap around at the end
  gCurrentPatternNumber = (gCurrentPatternNumber + 1) % ARRAY_SIZE( gPatterns);
}

#define RUNTIME 2048
#define LIGHTS 4
#define FADE 24

void cycling() {
  fadeToBlackBy( leds, NUM_LEDS, FADE);
  for( int i = 0; i < LIGHTS; i++ ) {
    int p = map((millis()+(i*(RUNTIME/LIGHTS)))%RUNTIME, 0, RUNTIME, 0, (NUM_LEDS/2));
    CRGB col = p > NUM_LEDS/4 ? CRGB::White : CRGB::Red;
    int p1 = NUM_LEDS/2-1 - p;
    int p2 = NUM_LEDS/2 + p;
    leds[p1] = col;
    leds[p2] = col;
  }
}

void blinken() 
{
  uint32_t ms = millis();  
  fill_solid( leds, NUM_LEDS, CHSV(gHue, 255, (((ms >> 9) % 2) == 0) ? 255 : 0) );
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
  fadeToBlackBy( leds, NUM_LEDS, 24);
  byte dothue = 0;
  for( int i = 0; i < 6; i++) {
    leds[beatsin16( i+7, 0, NUM_LEDS-1 )] |= CHSV(dothue, 200, 255);
    dothue += 32;
  }
}
