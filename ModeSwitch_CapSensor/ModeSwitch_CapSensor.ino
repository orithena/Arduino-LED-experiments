#include <FastLED.h>
#include <avr/pgmspace.h>

//#define DEBUG
//#define DEBUGIR
#define MODE_MAX 3
char* mode_names[] = { "Demo", "Tetris", "Soundfire", "Game of Life" };

#define DEMO_TEXT_COUNT 4
//PROGMEM char* const demo_text_lines[] = { "Is this the real life?", "Is this just fantasy?", "Caught in a landslide", "No escape from reality" };

int current_mode;
int current_demo_mode;
int demo_text_counter = 0;

void setup() {
#ifdef DEBUG
  Serial.begin(9600);
#endif
  randomSeed(analogRead(A5));
  sensor_setup();
  led_setup();
  switch_current_mode(3);
}

void loop() {
#ifdef DEBUG
  delay(100);
#endif
  if( read_sensor() ) {
    switch_current_mode(current_mode + 1);
  }
  int next_demo_mode = ((millis() >> 18) % MODE_MAX) + 1;
  if( current_demo_mode != next_demo_mode ) {
    current_demo_mode = next_demo_mode;
  //  ShowText(demo_text_lines[demo_text_counter]);
    demo_text_counter = (demo_text_counter + 1) % DEMO_TEXT_COUNT;
  }
  if( current_mode < 1 ) {
    mode_loop(current_demo_mode);
  } else {
    mode_loop(current_mode);
  }
}

void show_number(int number) {
  Clear();
  DrawNumberOneFrame(number, 10);
  FastLED.show();
  delay(500);
}

void switch_current_mode(int mode) {
  current_mode = mode % (MODE_MAX + 1);
  ShowText(mode_names[current_mode]);
  if(current_mode < 1) {
    switch_mode(current_demo_mode);
  } else {
    switch_mode(current_mode);
  }
}

void switch_mode(int mode) {
  switch(mode) {
  case 1:
    tetris_setup();
    break;
  case 2:
    firemic_setup();
    break;
  case 3:
    gol_setup();
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
  case 3:
    gol_loop();
    break;
  }
}


