void base_RGB(uint32_t ms) {
  byte x = ((ms >> 5) & 0x00FF) % 3;
  for( int i = 0; i < ADD_LEDS; i++ ) {
    real_leds[i] = CRGB(255 * (x == i), 255 * (x==((i+1)%3)), 255 * (x==((i+2)%3)));
  }
}

void base_Color(CRGB col) {
  for( int i = 0; i < ADD_LEDS; i++ ) {
    real_leds[i] = col;
  }
}

void base_ColorBlink(uint32_t ms, CRGB col) {
  for( int i = 0; i < ADD_LEDS; i++ ) {
    byte x = ((ms >> 2) & 0x01);
    real_leds[i] = CRGB(col.r*x, col.g*x, col.b*x);
  }
}
