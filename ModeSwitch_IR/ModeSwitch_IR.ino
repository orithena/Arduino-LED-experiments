#include <FastLED.h>

//#define DEBUG
//#define DEBUGIR
#define MODE_MAX 2

volatile int current_mode = 0;

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
#ifdef DEBUGIR
  Serial.begin(9600);
#endif
  NEC_IR_setup();
  led_setup();
  current_mode = 0;
}

void loop() {
  unsigned long code = NEC_IR_readlastcode();
  if( code > 0 ) {
#ifdef DEBUG
    Serial.println(code, HEX);
#endif
    interpret_code(code);
  }
#ifdef DEBUG
  delay(100);
#endif
  if( !IR_receiving() ) {
    mode_loop(current_mode);
  }
}

void interpret_code(unsigned long code) {
  if( code > 0 ) {
    switch_mode(current_mode + 1);
  }
  /* Something's wrong with IR receiving...
  switch(code & 0xFFFF) {
  case 0xC03F: //ON
    switch_mode(current_mode + 1);
    break;
  case 0x00FF: //Brightness UP
    FastLED.setBrightness(min(255, FastLED.getBrightness() + 32));
    break;
  case 0x807F: // Brightness DOWN
    FastLED.setBrightness(max(31, FastLED.getBrightness() - 32));
    break;
  case 0xFFFF: // Error
    show_number(99);
    break;
  }
  */
}

void show_number(int number) {
  Clear();
  DrawNumberOneFrame(number, 10);
  FastLED.show();
  delay(500);
}

void switch_mode(int mode) {
  current_mode = mode % (MODE_MAX + 1);
  if(current_mode < 1) {
    current_mode = 1;
  }
  switch(current_mode) {
  case 1:
    tetris_setup();
    break;
  case 2:
    firemic_setup();
    break;
  }

}

void mode_loop(int mode) {
  switch(mode) {
  case 1:
    tetris_loop();
    break;
  case 2:
    firemic_loop();
    break;
  }
}


