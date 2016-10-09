#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 10
// Where is the LED data line connected?
#define DATA_PIN 13

#define P_MODE 7
#define P_VAR_UP 8
#define P_HUE_UP 9
#define P_SAT_UP 10
#define P_VAL_UP 11

#define P_OFF 6
#define P_VAR_DOWN 5
#define P_HUE_DOWN 4
#define P_SAT_DOWN 3
#define P_VAL_DOWN 2

#define MODE_COUNT 7

// Define the array of leds
CRGB leds[NUM_LEDS];

byte mode = 0, variance = 255, hue = 0, sat = 255, val = 255, power = 1;
uint32_t last_mode_press = 0;
uint32_t last_button_press = 0;

void setup() { 
  // Initialize the LEDs
  for( int pin = 2; pin < 12; pin++ ) {
    pinMode(pin, INPUT_PULLUP);
  }
  pinMode(DATA_PIN, OUTPUT); 
  FastLED.addLeds<APA106, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

uint32_t doffset = 0;
uint16_t offset = 0;
uint32_t counter = 0;
void loop() { 
  uint32_t ms = millis();
  read_buttons(ms);

  for( int i = 0; i < NUM_LEDS; i++ ) {
  
    byte computed_hue = hue;
    byte computed_sat = sat;
    byte computed_val = val;
    double rad = ((offset+(i*10))&0x000000FF)*(2.0*PI/256.0);
    
    if( mode == 0 ) {
      computed_hue = hue + (byte)(variance * cos(rad));
    }
    if( mode == 1 || mode == 3 ) {
      computed_sat = (byte)((double)sat * (1.0 - (variance/255.0) * (( cos(rad) + 1.0) * 0.5) ) );
    }
    if( mode == 2 || mode == 3 ) {
      computed_val = (byte)((double)val * (1.0 - (variance/255.0) * (( sin(rad) + 1.0) * 0.5) ) );
    }
    if( mode == 3 ) {
      computed_hue = hue + (i*(255/(NUM_LEDS-1)));
    }
    if( mode == 4 ) {
      computed_hue = hue + offset + (i*(255/(NUM_LEDS-1)));
    }
    if( mode == 5 ) {
      computed_hue = hue + offset + (i*93);
    }
    if( mode == 6 ) {
      uint32_t time = ((ms - last_mode_press) >> 10) * 0.9375;
      uint32_t p1 = variance*14.116, 
               p2 = p1+60+(variance*2), 
               p3 = 14400 + (val*80), 
               p4 = p3+600;
      if( time < 1 || ((ms - last_button_press) >> 10) < 2 ) {
        byte red = (600*(i+1) < p1) 
                    ? 255 
                    : (600*i < p1) 
                       ? ((p1%600)*0.425) 
                       : 0;
        byte white = (3600*(i+1) < p3) 
                      ? 0 
                      : (3600*i < p3) 
                        ? 255-((p3%3600)*0.07) 
                        : 255;
        computed_hue = 0;
        computed_sat = red;
        computed_val = max(max(red, white), 32);
      } else if( time < p1 ) {
        computed_sat = 192;
        computed_val = 255;
      } else if( time < p2 ) {
        computed_hue = 0;
        computed_sat = 192 + ( (time-p1) * ( 63.0/(p2-p1)) );
        computed_val = 255 - ( (time-p1) * (225.0/(p2-p1)) );
      } else if( time < p3 ) {
        computed_hue = 0;
        computed_sat = 255;
        computed_val =  16;
      } else if( time < p4 ) {
        computed_sat = 255 - ( (time-p3) * (255.0/(p4-p3)) );
        computed_val =  32 + ( (time-p3) * (223.0/(p4-p3)) );
      } else {
        computed_sat =   0;
        computed_val = 255;
      }
    }

    leds[i] = CHSV(computed_hue,computed_sat,computed_val);

    if( power == 0 ) {
      FastLED.setBrightness(0);
    } else {
      FastLED.setBrightness(255);
    }

    if( counter == 0 ) {
      FastLED.show();
    }
    delay(1);
  }
  doffset = (doffset + variance);
  offset = (doffset>>8) & 0x000000FF;
  counter = (counter + 1) & 0x00000003;
}

void read_buttons(uint32_t ms) {
  if( digitalRead(P_OFF) == LOW ) {
    last_button_press = ms;
    power = 0;
  }
  if( digitalRead(P_MODE) == LOW ) {
    last_button_press = ms;
    if( power == 1 && last_mode_press < (ms - 100) ) {
      mode = (mode + 1) % MODE_COUNT;
    }
    power = 1;
    last_mode_press = ms;
  }
  if( digitalRead(P_HUE_UP) == LOW) {
    last_button_press = ms;
    hue++;
  }
  if( digitalRead(P_HUE_DOWN) == LOW) {
    last_button_press = ms;
    hue--;
  }
  if( digitalRead(P_SAT_UP) == LOW && sat < 255) {
    last_button_press = ms;
    sat++;
  }
  if( digitalRead(P_SAT_DOWN) == LOW && sat > 0) {
    last_button_press = ms;
    sat--;
  }
  if( digitalRead(P_VAL_UP) == LOW && val < 255) {
    last_button_press = ms;
    val++;
  }
  if( digitalRead(P_VAL_DOWN) == LOW && val > 0) {
    last_button_press = ms;
    val--;
  }
  if( digitalRead(P_VAR_UP) == LOW && variance < 255) {
    last_button_press = ms;
    variance++;
  }
  if( digitalRead(P_VAR_DOWN) == LOW && variance > 0) {
    last_button_press = ms;
    variance--;
  }
}
