#include <FastLED.h>

#define LED_PIN  6

#define COLOR_ORDER RGB
#define CHIPSET     WS2811

#define BRIGHTNESS 255

#define  MESSAGE "This is a little message inclubing a typo. If you can read this, you stared long enough onto these pixels to actually read all of it."

// Helper functions for an two-dimensional XY matrix of pixels.
// Simple 2-D demo code is included as well.
//
//     XY(x,y) takes x and y coordinates and returns an LED index number,
//             for use like this:  leds[ XY(x,y) ] == CRGB::Red;
//             No error checking is performed on the ranges of x and y.
//
//     XYsafe(x,y) takes x and y coordinates and returns an LED index number,
//             for use like this:  leds[ XY(x,y) ] == CRGB::Red;
//             Error checking IS performed on the ranges of x and y, and an
//             index of "-1" is returned.  Special instructions below
//             explain how to use this without having to do your own error
//             checking every time you use this function.
//             This is a slightly more advanced technique, and
//             it REQUIRES SPECIAL ADDITIONAL setup, described below.


// Params for width and height
const uint8_t kMatrixWidth = 10;
const uint8_t kMatrixHeight = 7;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
// Set 'kMatrixSerpentineLayout' to false if your pixels are
// laid out all running the same way, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//     .----<----<----<----'
//     |
//     5 >  6 >  7 >  8 >  9
//                         |
//     .----<----<----<----'
//     |
//    10 > 11 > 12 > 13 > 14
//                         |
//     .----<----<----<----'
//     |
//    15 > 16 > 17 > 18 > 19
//
// Set 'kMatrixSerpentineLayout' to true if your pixels are
// laid out back-and-forth, like this:
//
//     0 >  1 >  2 >  3 >  4
//                         |
//                         |
//     9 <  8 <  7 <  6 <  5
//     |
//     |
//    10 > 11 > 12 > 13 > 14
//                        |
//                        |
//    19 < 18 < 17 < 16 < 15
//
// Bonus vocabulary word: anything that goes one way
// in one row, and then backwards in the next row, and so on
// is call "boustrophedon", meaning "as the ox plows."


// This function will return the right 'led index number' for
// a given set of X and Y coordinates on your matrix.
// IT DOES NOT CHECK THE COORDINATE BOUNDARIES.
// That's up to you.  Don't pass it bogus values.
//
// Use the "XY" function like this:
//
//    for( uint8_t x = 0; x < kMatrixWidth; x++) {
//      for( uint8_t y = 0; y < kMatrixHeight; y++) {
//
//        // Here's the x, y to 'led index' in action:
//        leds[ XY( x, y) ] = CHSV( random8(), 255, 255);
//
//      }
//    }
//
//
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if ( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if ( kMatrixSerpentineLayout == true) {
    if ( !(y & 0x01)) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } 
    else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}


// Once you've gotten the basics working (AND NOT UNTIL THEN!)
// here's a helpful technique that can be tricky to set up, but
// then helps you avoid the needs for sprinkling array-bound-checking
// throughout your code.
//
// It requires a careful attention to get it set up correctly, but
// can potentially make your code smaller and faster.
//
// Suppose you have an 8 x 5 matrix of 40 LEDs.  Normally, you'd
// delcare your leds array like this:
//    CRGB leds[40];
// But instead of that, declare an LED buffer with one extra pixel in
// it, "leds_plus_safety_pixel".  Then declare "leds" as a pointer to
// that array, but starting with the 2nd element (id=1) of that array:
//    CRGB leds_with_safety_pixel[41];
//    const CRGB* leds( leds_plus_safety_pixel + 1);
// Then you use the "leds" array as you normally would.
// Now "leds[0..N]" are aliases for "leds_plus_safety_pixel[1..(N+1)]",
// AND leds[-1] is now a legitimate and safe alias for leds_plus_safety_pixel[0].
// leds_plus_safety_pixel[0] aka leds[-1] is now your "safety pixel".
//
// Now instead of using the XY function above, use the one below, "XYsafe".
//
// If the X and Y values are 'in bounds', this function will return an index
// into the visible led array, same as "XY" does.
// HOWEVER -- and this is the trick -- if the X or Y values
// are out of bounds, this function will return an index of -1.
// And since leds[-1] is actually just an alias for leds_plus_safety_pixel[0],
// it's a totally safe and legal place to access.  And since the 'safety pixel'
// falls 'outside' the visible part of the LED array, anything you write
// there is hidden from view automatically.
// Thus, this line of code is totally safe, regardless of the actual size of
// your matrix:
//    leds[ XYsafe( random8(), random8() ) ] = CHSV( random8(), 255, 255);
//
// The only catch here is that while this makes it safe to read from and
// write to 'any pixel', there's really only ONE 'safety pixel'.  No matter
// what out-of-bounds coordinates you write to, you'll really be writing to
// that one safety pixel.  And if you try to READ from the safety pixel,
// you'll read whatever was written there last, reglardless of what coordinates
// were supplied.

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* leds( leds_plus_safety_pixel + 1);
byte buf[ 2 ][ kMatrixWidth+2 ][ kMatrixHeight+4 ];
byte bufi = 0;
int mic_cur, mic_old, mic_delta, mic_mean, mic_mean_delta, mic_mean_old, mic_mean_far = 0;
uint32_t counter = 0;

uint16_t XYsafe( int x, int y)
{
  if ( x >= kMatrixWidth) return -1;
  if ( y >= kMatrixHeight) return -1;
  if ( x < 0) return -1;
  if ( y < 0) return -1;
  return XY(x, y);
}


// Demo that USES "XY" follows code below

void loop()
{
  counter++;
  mic_cur = analogRead(A1);
  mic_delta = mic_cur - mic_old;
  //if( abs(mic_delta) < 2 ) mic_delta = 0;
  mic_mean += mic_delta / 3.0;
  mic_mean_delta = mic_mean - mic_mean_old;
  mic_mean_far += mic_delta / 64.0;
  mic_old = mic_cur;
  mic_mean_old = mic_mean;

  int length = strlen(MESSAGE) * 6 + 20;
  uint32_t ms = millis();
  ms = ms & 0x0003FFFF;

  //FireLoop();
  if( (counter/kMatrixWidth) % 3 == 0 ) MicroCenter(counter);
  FastLED.show();
}

void MicroShow(uint32_t ms) {
  //if(mic_delta > 8) { leds[ XYsafe(0,0) ] = CRGB(0,0,255); }
/*  for( byte x = kMatrixWidth - 1; x > 0; x-- ) {
    for( byte y = 0; y < kMatrixHeight; y++ ) {
      leds[ XYsafe(x,y) ] = leds[ XYsafe(x-1, y) ];
    }
  }
*/
  int data = mic_mean_far - mic_mean;
  int l = (abs(data)*32);
  for( int i = 0; i < kMatrixHeight; i++) {
    leds[ XYsafe(ms % kMatrixWidth,i) ] = CRGB(min(255, l*(data < 0)),0, min(255, l*(data > 0)) );
    l = max(0,l-256);
  }
}

void MicroCenter(uint32_t ms) {
  int data = mic_mean_far - mic_mean_delta;
  int l = (data*16);
  for( int i = 0; i < kMatrixHeight/2; i++) {
    leds[ XYsafe(ms % kMatrixWidth,(kMatrixHeight/2)+i+1) ] = CRGB(0,0, min(255, l*(data>0) ));
    l = max(0,l-256);
  }
  l = -(data*16);
  for( int i = 0; i < kMatrixHeight/2; i++) {
    leds[ XYsafe(ms % kMatrixWidth,(kMatrixHeight/2)-i) ] = CRGB(min(255, l*(data<=0) ),0,0);
    l = max(0,l-256);
  }
  l = mic_mean_far - mic_delta;
  l *= 16;
  for( int i = 0; i < kMatrixWidth; i++) {
    leds[ XYsafe(i,0) ] = CRGB(0,min(255, l),0);
    l = max(0,l-256);
  }
}

void SinesLoop(uint32_t ms) {
  Clear();
  for( int x = 0; x < kMatrixWidth; x++) {
    leds[ XYsafe(x, ((1.2+sin((ms+x)/4.0))/2.4)*kMatrixHeight) ].r = 255;
    leds[ XYsafe(x, ((1.2+sin((ms+x+7)/4.0))/2.4)*kMatrixHeight) ].g = 255;
    leds[ XYsafe(x, ((1.2+sin((ms+x+15)/4.0))/2.4)*kMatrixHeight) ].b = 255;
  }
}

CRGB palette(byte shade) {
  double r = 1-cos(((shade/255.0)*PI)/2);
  double g = ((1-cos(((shade/255.0)*6*PI)/2))/2) * (r*r*0.5);
  double b = (1-cos((max(shade-128,0)/128.0)*0.5*PI)) * r;
  return CRGB(r*255, g*255 , b*255);
}

#define COOLDOWN 12
uint16_t firecount = 0;
void FireLoop() {
  if( firecount % 4 <= 1 ) {
    for( byte x = 0; x < kMatrixWidth+2; x++ ) {
      byte r = random(3);
      buf[bufi][x][kMatrixHeight+2] = (byte)0x31 << r;
      buf[bufi][x][kMatrixHeight+3] = (byte)0x31 << r+1;
    }
  }
  if( mic_mean_delta > 16 ) {
    byte w = min(4, mic_mean_delta/16) + 1;
    byte o = 1+w+random(kMatrixWidth-w-1);
    for( byte x = o-w; x <= o+w; x++) {
      for( byte y = 1; y <= 3; y++)  {
        buf[bufi][x][kMatrixHeight+y] = 255-((y-1)*64);
      }
    }
  }
  for( byte x = 1; x <= kMatrixWidth; x++ ) {
    for( int y = 0; y <= kMatrixHeight+2; y++ ) {
      int delta = (buf[bufi][x-1][y] + buf[bufi][x+1][y] + buf[bufi][x][y-1] + buf[bufi][x][y+1]) / 4;
      if( delta <= COOLDOWN ) delta = COOLDOWN;
      buf[bufi^1][x][y-1] = (delta - COOLDOWN);
    }
  }
  bufi ^= 1;
  for( byte x = 0; x < kMatrixWidth; x++ ) {
    for( byte y = 0; y < kMatrixHeight; y++ ) {
      leds[ XYsafe(x, y) ] = palette( buf[bufi][x+1][kMatrixHeight - y - 1] );
    }
  }
  firecount++;
}

void PaletteTestLoop(uint32_t ms) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      leds[ XYsafe(x,y) ] = palette( ((ms/100) + (x*kMatrixWidth) + y) % 255 );
    }
  }
}

void Clear() {
  for ( byte y = 0; y < kMatrixHeight; y++) {
    for ( byte x = 0; x < kMatrixWidth; x++) {
      leds[ XYsafe(x, y)]  = CHSV((16*y)+(47*x), 255, 42);
    }
  }
}

void setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
  pinMode(A1, INPUT);
  mic_mean_far = mic_mean = mic_old = mic_mean_old = analogRead(A1);
}




