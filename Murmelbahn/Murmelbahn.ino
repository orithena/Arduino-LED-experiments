#include "FastLED.h"

// How many leds in your strip?
#define NUM_LEDS 12

// Where is the LED data line connected?
#define DATA_PIN 2
#define TRACK_BASE 5
#define TRACKS 5
#define TRACK_0 5
#define TRACK_1 6
#define TRACK_2 7
#define TRACK_3 8
#define TRACK_4 9
#define POWER_PIN 10

#define TRACK_ACTIVITY_INCREMENT 12

// Define the array of leds
CRGB leds[NUM_LEDS];

byte track[5] = {0,0,0,0,0};
uint32_t ms = 0;
uint32_t loop_counter = 0;
uint32_t last_power = 0;
boolean power = true;

int track_energy = 0;
int soft_track_energy = 0;
int fading_track_energy = 0;
byte rotator = 0;
double rad_rotator = 0;
byte cos_rotator = 0;

void setup() { 
  // Initialize the LEDs
  pinMode(DATA_PIN, OUTPUT); 
  for( int a = 0; a < TRACKS; a++) {
    pinMode(TRACK_BASE + a, INPUT_PULLUP); 
  }
  pinMode(POWER_PIN, INPUT_PULLUP); 
  
  FastLED.addLeds<APA106, DATA_PIN, RGB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
}

void loop() {
  ms = millis();
  loop_counter++;
  
  for( int a = 0; a < 10; a++ ) {
    read_power();
  
    read_tracks();
    
    delay(2);
  }
  
  calculate_common_variables();

  light_track_leds();

  background_lights();
  
  FastLED.show();
}

void calculate_common_variables() {
  track_energy = 0;
  for( int a = 0; a < TRACKS; a++ ) { track_energy += track[a]; }
  
  int delta = track_energy - soft_track_energy;
  soft_track_energy += ( delta > 0 ? (delta * 0.75) : (delta * 0.25) );

  if( track_energy > fading_track_energy ) {
    fading_track_energy = track_energy;
  } else if( (fading_track_energy > 0) && ((loop_counter & 0x00000003) == 0) ) {
    fading_track_energy -= 1;
  }
  
  rotator += (fading_track_energy >> 4) & 0x07;
  rad_rotator = rotator*0.02463994;
  cos_rotator = (byte)(127.5+(127.5*cos(rad_rotator)));
}

int hue, sat, val;

void background_lights() {
  for( int a = 0; a < 8; a++ ) {
    int led = (( a < 4 ) ? a : (a + 4));
    hue = (rotator) + ((ms>> (7+((cos_rotator>>6)&0x03)) ) + (a*((rotator>>2)+(soft_track_energy>>6)))) & 0xFF;
    sat = 255 - min(fading_track_energy >> 4, 255);
    val = 255 - min(fading_track_energy >> 2, 255);
    leds[led] = CHSV(hue, sat, val);
  }
}

void read_power() {
  if( digitalRead(POWER_PIN) == LOW && ms - last_power > 100 ) {
    power = !power;
    last_power = ms;
    FastLED.setBrightness(power*255);
  }
}

void read_tracks() {
  reduce_track_energy();
  for( int a = 0; a < TRACKS; a++ ) {
    if( digitalRead(TRACK_BASE + a) == LOW ) {
      if( track[a] < (255-TRACK_ACTIVITY_INCREMENT) ) {
        track[a] = 255;
      } else {
        track[a] += TRACK_ACTIVITY_INCREMENT;
      }
    }
  }
}

void light_track_leds() {
  for( int a = 0; a < TRACKS-1; a++ ) {
    int bright = (fading_track_energy >> 2) + track[a];
    if( bright > 0 ) {
      leds[7-a] = CHSV((cos_rotator+(a*(soft_track_energy>>4))) & 0xFF, 255, bright > 255 ? 255 : bright);
    } else {
      int b = 4+a;
      int j = ( (~b)<<3 | ((( (b>>1) ^ b )&1)<<1) )&0x0F;
      int i = ( (~j & ~0x0C) | ((b<<2) & 0x08) )&0x0F;
      leds[b].r = (leds[i].r + leds[j].r) >> 3;
      leds[b].g = (leds[i].g + leds[j].g) >> 3;
      leds[b].b = (leds[i].b + leds[j].b) >> 3;
    }
  }
}

void reduce_track_energy() {
  for( int a = 0; a < TRACKS; a++ ) {
    if( track[a] > 0 ) {
      track[a] -= 1;
    }
  }
}

