#include "FastLED.h"


/* LED layout:
 *   +----------------+
 *   |                |
 *   | 0     7     11 |
 *   | 1           10 |
 *   | 2  5     6   9 |
 *   | 3            8 |
 *   |       4        |
 *   |                |
 *   +----------------+
 * 0..3  are the "left background LEDs", 
 * 4..7  are the "track LEDs", 
 * 8..11 are the "right background LEDs".
 *
 * Track sensors are counted from top to bottom from 0..4
 * Tracks 0..3 have the "corresponding" track LEDs 7..4 (yes, in reverse order)
 * Track 4 does not have a "corresponding" track LED
 *
 * In the "pause animation", LED 7 is derived from LEDs 0 and 11, 6 from 9 and 10, 5 from 1 and 2, and 4 from 3 and 8.
 */

 /* Remember, ">> 1" (shift bitfield right) equals "divide by 2", ">> 2" = "divide by 4", ">> n" = "divide by 2^n".
  * Bitfield shifting is simply faster to calculate than true division on a processor that divides by repeated subtraction,
  * therefore I used it a lot in this code.
  */

// ===== Definitions

// Is the Debug Mode on? (a.k.a. Serial Output?)
#define DEBUG

// How many leds in your strip?
#define NUM_LEDS 12

// Where is the LED data line connected?
#define DATA_PIN 2

// lowest Pin of track sensors (assuming all track sensors are on incrementing pins)
#define TRACK_BASE 5
// number of track sensors
#define TRACKS 5
// individual pins of track sensors
#define TRACK_0 5
#define TRACK_1 6
#define TRACK_2 7
#define TRACK_3 8
#define TRACK_4 9

// power button pin
#define POWER_PIN 10

// how much to increase the "energy metric" per track if a track sensor is active
#define TRACK_ACTIVITY_INCREMENT 12
#define TRACK_ACTIVITY_DECREMENT 1
// delay loop presets -- LEDs are updated every (DELAY_LOOP_COUNT*DELAY_LOOP_WAIT) ms
#define DELAY_LOOP_COUNT 20
// sensors are read every DELAY_LOOP_WAIT ms
#define DELAY_LOOP_WAIT 2

// wait 500 ms before re-reading power button
#define POWER_BUTTON_REPEAT_DELAY 500

// how fast should soft_track_energy increase?
#define SOFT_TRACK_ATTACK_FACTOR 0.75
// how fast should soft_track_energy decline?
#define SOFT_TRACK_FADE_FACTOR 0.25
// after how many loops should the fading_track_energy decrease by 1? (must be a number that satisfies (2^n)-1)
#define FADING_TRACK_MODULUS 0x00000003

// ===== Global Variables

CRGB leds[NUM_LEDS];                // Define the array of leds

byte track[5] = {0,0,0,0,0};        // "energy metric" per track

uint32_t ms = 0;                    // we want to read the millisecond counter only once per loop
uint32_t loop_counter = 0;          // we want to have a loop counter that reliably increases by exactly 1 per loop

uint32_t last_power = 0;            // we need to save when the power button was pressed last
boolean power = true;               // current power state

// === common animation variables and their derivations
int track_energy = 0;               // sum of current track energy metrics => sum(track[0..4])
int soft_track_energy = 0;          // slightly smoothed track energy (short attack, medium fadeout)
int fading_track_energy = 0;        // sawtoothed track energy (immediate attack, long fadeout)
byte rotator = 0;                   // rotates in 0..255 on activity on the tracks (increases in every loop with fading_track_energy)
double rad_rotator = 0;             // rotator in radians => rotates in 0..(2*PI)
byte cos_rotator = 0;               // cosine of rad_rotator, scaled from -1.0..1.0 to 0..255


// executed once at boot (power is plugged in or reset button is pressed)
void setup() { 
  // Initialize the LED Data Pin
  pinMode(DATA_PIN, OUTPUT); 
  // Initialize the track sensors with internal pullup resistor (i.e. pin LOW => activity on track)
  for( int a = 0; a < TRACKS; a++) {
    pinMode(TRACK_BASE + a, INPUT_PULLUP); 
  }
  // Initialize Power Button pin
  pinMode(POWER_PIN, INPUT_PULLUP); 

#ifdef DEBUG
  Serial.begin(115200);
#endif
  
  // Initialize FastLED library
  FastLED.addLeds<APA106, DATA_PIN, RGB>(leds, NUM_LEDS);
  // set initial brightness to full
  FastLED.setBrightness(255);
}

// executed endlessly...
void loop() {
  ms = millis();                    // read current milliseconds
  loop_counter++;                   // increase loop counter
  
  for( int a = 0; a < DELAY_LOOP_COUNT; a++ ) {   // we read the sensors more often than we update the LEDs
    read_power();                   // read power button pin and switch power state, if necessary
  
    read_tracks();                  // read track sensors into track[] array
    
    delay(DELAY_LOOP_WAIT);
  }
  
  calculate_common_variables();     // derive common variables from track[] array

  background_lights();              // calculate the eight background LEDs

  light_track_leds();               // calculate the four center LEDs

#ifdef DEBUG
  for( int a = 0; a < NUM_LEDS; a++ ) {
    PrintHex8(&(leds[a].r), 1);
    PrintHex8(&(leds[a].g), 1);
    PrintHex8(&(leds[a].b), 1);
    Serial.print("  ");
  }
  Serial.println();
#endif
  
  FastLED.show();                   // send leds[] array out to the LED chain
}

void PrintHex8(uint8_t *data, uint8_t length) // prints 8-bit data in hex with leading zeroes
{
     char tmp[16];
       for (int i=0; i<length; i++) { 
         sprintf(tmp, "%.2X",data[i]); 
         Serial.print(tmp); Serial.print(" ");
       }
}

void calculate_common_variables() {
  // sum up current track energy
  track_energy = 0;
  for( int a = 0; a < TRACKS; a++ ) { track_energy += track[a]; }
  
  // smooth out the track energy over time
  int delta = track_energy - soft_track_energy;
  soft_track_energy += ( delta > 0 ? (delta * SOFT_TRACK_ATTACK_FACTOR) : (delta * SOFT_TRACK_FADE_FACTOR) );

  if( track_energy > fading_track_energy ) {
    // fading_track_energy always rises immediately...
    fading_track_energy = track_energy;
  } else if( (fading_track_energy > 0) && ((loop_counter & FADING_TRACK_MODULUS) == 0) ) {
    // ...and declines veery slowly.
    fading_track_energy -= 1;
  }
  
  // the rotator simply increases if some (but not too few) energy is left on the fader
  rotator += (fading_track_energy >> 4) & 0x07;
  // scale rotator to radians: (0..255) * (1/(255/(2*PI)))
  rad_rotator = rotator*0.02463994;
  // calculate cosine of rad_rotator and scale from -1.0 .. 1.0 up to 0..255
  cos_rotator = (byte)(127.5+(127.5*cos(rad_rotator)));
}


void background_lights() {
  for( int a = 0; a < 8; a++ ) {
    int led = (( a < 4 ) ? a : (a + 4));   // map a, which is (0..7) to actual background led number (0..3,8..11)
    int hue, sat, val;
    // calculate this LEDs color
    hue = (rotator) + ((ms>> (5+((cos_rotator>>6)&0x03)) ) + (a*((rotator>>2)+(soft_track_energy>>6)))) & 0xFF;
    // calculate this LEDs saturation
    sat = 255 - min(fading_track_energy >> 4, 255);
    // calclulate this LEDs value
    val = 255 - min(fading_track_energy >> 2, 255);
    leds[led] = CHSV(hue, sat, val);
  }
}

void read_power() {
  // if button is pressed and the last time we switched power state was more than POWER_BUTTON_REPEAT_DELAY ms ago...
  if( digitalRead(POWER_PIN) == LOW && ms - last_power > POWER_BUTTON_REPEAT_DELAY) {
    // switch power state
    power = !power;
    // save timestamp of power switching
    last_power = ms;
    // power is a boolean, i.e. false == 0 and true == 1, therefore we can do this little trick:
    FastLED.setBrightness(power*255);
  }
}

void light_track_leds() {
  for( int a = 0; a < TRACKS-1; a++ ) {
    // bright is > 0 if there is some energy left on the track specified by a
    int bright = (fading_track_energy >> 2) + track[a];
    if( bright > 0 ) {
      // calculate animation frame when at least one track is active
      leds[7-a] = CHSV((cos_rotator+(a*(soft_track_energy>>4))) & 0xFF, 255, bright > 255 ? 255 : bright);
    } else {
      // if all tracks are inactive and fading_track_energy is faded out, we switch to the "pause animation" on the track LEDs
      // b is the actual LED number
      int b = 4+a;

      if( rotator & 0x01 == 0 ) {
        // this implementation here simply takes a dimmed average of the background LEDs

        // i and j are the LED numbers where we take the average from. 
        // this is a bit of bit operation magic to fulfill this mapping:
        //  LED 7 takes the RGB average of LEDs 11 and 0
        //  LED 6 takes the RGB average of LEDs 9 and 10
        //  LED 5 takes the RGB average of LEDs 1 and 2
        //  LED 4 takes the RGB average of LEDs 3 and 8
        int j = ( (~b)<<3 | ((( (b>>1) ^ b )&1)<<1) )&0x0F;
        int i = ( (~j & ~0x0C) | ((b<<2) & 0x08) )&0x0F;
        
        // this calculates the average of two LEDs, dims it and puts it out to the "track LEDs"
        // remember, ">> 1" (shift bitfield right) equals "divide by 2", ">> 2" = "divide by 4", ...
        leds[b].r = (leds[i].r + leds[j].r) >> 3;
        leds[b].g = (leds[i].g + leds[j].g) >> 3;
        leds[b].b = (leds[i].b + leds[j].b) >> 3;
      } else {
        // tracks lights off
        leds[b] = CRGB(0,0,0);      
      }
    }
  }
}

void read_tracks() {
  // first, fade out per-track energy
  reduce_track_energy();
  for( int a = 0; a < TRACKS; a++ ) {
    if( digitalRead(TRACK_BASE + a) == LOW ) {
      // if track a is active, increment it by TRACK_ACTIVITY_INCREMENT, capped at 255
      if( track[a] < (255-TRACK_ACTIVITY_INCREMENT) ) {
        track[a] = 255;
      } else {
        track[a] += TRACK_ACTIVITY_INCREMENT;
      }
    }
  }
}

void reduce_track_energy() {
  // decrease all tracks by TRACK_ACTIVITY_DECREMENT, but stop at 0
  for( int a = 0; a < TRACKS; a++ ) {
    if( track[a] > 0 ) {
      if( track[a] < TRACK_ACTIVITY_DECREMENT ) {
        track[a] = 0;
      } else {
        track[a] -= TRACK_ACTIVITY_DECREMENT;
      }
    }
  }
}

