
//#define DEBUG
#define LED_PIN               6            // LED chain data pin
#define COLOR_ORDER RGB                    // LED chip color order
#define CHIPSET     WS2811                 // LED chipset
#define BRIGHTNESS 72                     // LED overall brightness

// Params for width and height of the LED Matrix
const uint8_t kMatrixWidth = 10;
const uint8_t kMatrixHeight = 7;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
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

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* leds( leds_plus_safety_pixel + 1);

uint16_t XYsafe( int x, int y)
{
  if ( x >= kMatrixWidth) return -1;
  if ( y >= kMatrixHeight) return -1;
  if ( x < 0) return -1;
  if ( y < 0) return -1;
  return XY(x, y);
}

void Clear() {
  for ( byte y = 0; y < kMatrixHeight; y++) {
    for ( byte x = 0; x < kMatrixWidth; x++) {
      leds[ XYsafe(x, y)]  = CHSV((16*y)+(47*x), 255, 42);
    }
  }
}

void led_setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
}
