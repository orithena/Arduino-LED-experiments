#define FASTLED_ALLOW_INTERRUPTS 0
#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 32
// MAX_VAL/NUM_LEDS = 255/12.0s = 21.25
#define STEP 3.4
// Where is the LED data line connected?
#define DATA_PIN 0
#define BTN_0 5
#define BTN_1 8
#define BTN_2 10

#define MODE_COUNT 4
#define BUTTON_MODE_COUNT 4

// Define the array of leds
CRGB leds[NUM_LEDS];

// internal status variables
byte brightness = 192; // 16..255
byte special = 128;    // 0..255
boolean power = true;  // true/false
double spd = 1.0;      // -16.0..16.0
byte mode = 0;         // 0..(MODE_COUNT-1)
byte preset_mode = MODE_COUNT;

// running status variables
double offset = 0.0;

// timing
uint32_t ms = 0;

// button status variables
uint32_t last_button_0 = 1001;
uint32_t last_button_1 = 1001;
int last_return_0 = 0;
int button_mode = 0;

void setup() { 
  // Initialize the pins
  //pinMode(BTN_0, INPUT_PULLUP); 
  //pinMode(BTN_1, INPUT_PULLUP); 
  //pinMode(BTN_2, INPUT_PULLUP); 
  //pinMode(DATA_PIN, OUTPUT); 
  
  // initialize LED library... well I needed to patch it a bit, cause APA106 LEDs weren't supported
  // this goes to chipset.h to the correct place:
  // template <uint8_t DATA_PIN, EOrder RGB_ORDER = RGB>
  // class APA106Controller800Khz : public ClocklessController<DATA_PIN, NS(330), NS(520), NS(280), RGB_ORDER> {}; 
  FastLED.addLeds<WS2812B, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

void loop() { 
  // just so we don't need a function call each time we need timing
  ms = millis();
  
  mode = (preset_mode == MODE_COUNT ? (ms >> 16) % MODE_COUNT : preset_mode);

  switch(mode) {
  case 0: calc_mode_0(); break;
  case 1: calc_mode_1(); break;
  case 2: calc_mode_2(); break;
  case 3: calc_mode_3(); break;
  }    
  // send LED array out to LEDs
  FastLED.show();
  
  // increase offset  
  offset = (offset + spd);
  
  interpret_buttons(); 

  // wait a bit :)
  delay(10);
}

inline void calc_mode_0() {
  for( int i = 0; i < NUM_LEDS; i++ ) {
    byte hue = ((int)(offset) & 0x00FF) + (i*STEP);
    CRGB col = CHSV( hue, 255, brightness); 
    leds[i] = col;
  }
}

inline void calc_mode_1() {
  for( int i = 0; i < NUM_LEDS; i++ ) {
    byte hue = (int)(offset+(cos(((i+(ms>>10))%12)*0.5235)*(16+(special>>1)))) & 0x000000FF;
    CRGB col = CHSV( hue, 255, brightness); 
    leds[i] = col;
  }
}

inline void calc_mode_2() {
  for( int i = 0; i < NUM_LEDS; i++ ) {
    byte hue = (int)(offset+(cos(((i+(ms>>10))%12)*0.5235)*32)) & 0x000000FF;
    byte sat = 191 + ((int)((cos(((i+(ms>>11))%12)*0.5235)*(special>>1))) & 0x0000003F);
    byte val = ((int)((cos(((i+(ms>>11))%12)*0.5235)*brightness)) & 0x000000FF);
    CRGB col = CHSV( hue, sat, val); 
    leds[i] = col;
  }
}

inline void calc_mode_3() {
  if( random( 1+ ((255-special)>>2) ) == 0 ) {
    CRGB col = CHSV(offset + random(16), 255, 127+random(brightness>>1));
    byte led = random(12);
    CRGB cur = leds[led];
    if( cur.r + cur.g + cur.b < col.r + col.g + col.b ) {
      leds[led] = col;
    }
  }
  if( ((ms>>3) & 0x00000001) == 0 ) {
    for( int i = 0; i < NUM_LEDS; i++ ) {
      if( spd < 0 ) {
        leds[i] %= 256+(int)(spd*8);
      } else {
        leds[i].subtractFromRGB(1+(int)(spd*0.5));
      }
    }  
  }
}

inline void interpret_buttons() {
  // get buttons
  int button = check_button();
  
  if( button == 127 ) {
    // long press on BTN0 
    power = !power;
    button_mode = 0;
    FastLED.setBrightness(power * 255);
  }
  if( button == 1 && preset_mode < MODE_COUNT ) {
    // button_mode == 0 -> BTN1 increases mode
    preset_mode = preset_mode + 1;
  }
  if( button == 2 && preset_mode > 0 ) {
    // button_mode == 0 -> BTN2 decreases mode
    preset_mode = preset_mode - 1;
  }
  if( button == 3 && spd < 16.0 ) {
    // button_mode == 1 -> BTN1 increases speed
    spd = spd + 0.0625;
  }
  if( button == 4 && spd > -16.0 ) {
    // button_mode == 1 -> BTN2 decreases speed
    spd = spd - 0.0625;
  }
  if( button == 5 && brightness < 255 ) {
    // button_mode == 2 -> BTN1 increases brightness
    brightness = brightness + 1;
  }
  if( button == 6 && brightness > 16 ) {
    // button_mode == 2 -> BTN2 decreases brightness
    // we don't go down to "all off" here. That's what power is for.
    brightness = brightness - 1;
  }
  if( button == 7 ) {
    // button_mode == 3 -> BTN1 increases special
    special = special + 1; 
  }
  if( button == 8) {
    // button_mode == 3 -> BTN2 decreases special
    special = special - 1;
  }
}

inline int check_button() {
  int ret = -1;
  int btn0 = digitalRead(BTN_0);
  if( btn0 == LOW && (ms - last_button_0) > 1000 ) {
    // BTN0 is pressed for longer than 1000ms
    if( last_return_0 != 127 ) {
      // we return "power off" only once!
      ret = 127;
    }
    last_return_0 = 127;
  } else if( btn0 == LOW ) {
    // BTN0 is (still) pressed
    last_return_0 = -2;
  } else if( last_return_0 == -2 && (ms - last_button_0) > 50 ) {
    // BTN0 has just been released and was pressed for more than 50ms
    // -> cycle through button_mode, but don't return some value
    button_mode = (button_mode + 1) % BUTTON_MODE_COUNT;
    last_return_0 = 0;
  } else {
    // BTN0 is not pressed, we need to save when
    last_button_0 = ms;
    last_return_0 = -1;
  }
  
  if( ms - last_button_1 > (button_mode == 0 ? 300 : 20) ) {
    // the last time that BTN1 or BTN2 has been pressed is more than 20ms ago.
    if( digitalRead(BTN_1) == LOW ) {
      // BTN1 has been pressed
      ret = (button_mode * 2) + 1;
      last_button_1 = ms;
    }
    if( digitalRead(BTN_2) == LOW ) {
      // BTN2 has been pressed
      ret = (button_mode * 2) + 2;
      last_button_1 = ms;
    }
  }
  return ret;
}

