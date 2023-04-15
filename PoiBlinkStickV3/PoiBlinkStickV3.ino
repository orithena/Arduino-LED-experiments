#define FASTLED_ALLOW_INTERRUPTS 0
//#define DEBUG
//#define DEBUG_PIR
//#define DEBUG_SLEEP

#include "FastLED.h"

FASTLED_USING_NAMESPACE

#if defined(FASTLED_VERSION) && (FASTLED_VERSION < 3001000)
#warning "Requires FastLED 3.1 or later; check github for latest code."
#endif

// How many leds in your strip?
#define NUM_LEDS 12
#define ADD_LEDS 3
// Where is the LED data line connected?
#define DATA_PIN 6
#define BUTTON_PIN 8

#define TEXT_STRING "CCCAMP19"

// Define the array of leds
CRGB real_leds[NUM_LEDS+ADD_LEDS];
CRGB* leds = real_leds + ADD_LEDS;
uint32_t last_button = 0;
int user_mode = 0;

int oldx = 0;
byte tcol = 1;

void setup() { 
  // Initialize the LEDs
  Serial.begin(230400);
  pinMode(DATA_PIN, OUTPUT); 
  pinMode(BUTTON_PIN, INPUT_PULLUP); 
  FastLED.addLeds<APA106, DATA_PIN, RGB>(real_leds, NUM_LEDS+ADD_LEDS);
  FastLED.setBrightness(255);
  last_button = millis();
}

typedef void (*SimplePatternList[])(uint32_t ms);
SimplePatternList patterns = { 
  loop_demo,
  loop_HeartRGB,
  loop_RainbowSpiral2,
  loop_RandomStars,
  loop_RGB_sawtooth_inv, 
  loop_Rainbow_Dec,  
  loop_Lissajou,
  loop_Stars2, 
//  loop_Text,
  loop_RainbowSpiral,
  loop_Pacman, 
  loop_RGB_sawtooth, 
  loop_Lissajou,
//  loop_Pong,
//  loop_Heart,
  loop_Rainbow, 
//  loop_RGBBlink, 
//  loop_Rainbow_Inc,  
//  loop_Red,
//  loop_Stars 
};
#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))
#define MODE_COUNT ARRAY_SIZE( patterns )

void loop() {
  uint32_t ms = millis();
  if( check_button() ) {
    user_mode = (user_mode + 1) % MODE_COUNT;
  }
  patterns[user_mode](ms);
  FastLED.show();
}

void loop_demo(uint32_t ms) {
  patterns[((ms >> 14) % (MODE_COUNT - 1)) + 1](ms);
}


void Clear() {
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CRGB(0,0,0);
  }
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
