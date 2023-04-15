
static uint16_t heart[] = {
  0b0000001110000000,
  0b0000011111100000,
  0b0000111111111000,
  0b0000111111111100,
  0b0000001111111111,
  
  0b0000111111111100,
  0b0000111111111000,
  0b0000011111100000,
  0b0000001110000000,
  0b0000000000000000
};

void loop_Heart(uint32_t ms) {
  int idx = (((ms>>3) & 0x0FFF) % 10);
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CRGB(255 * ((heart[idx] >> (NUM_LEDS - 1 - i)) & 0x0001),0,0);
  }  
  base_Color(CRGB(255,0,0));
}

byte heart_color = 0;
void loop_HeartRGB(uint32_t ms) {
  int idx = (((ms>>3) & 0x0FFF) % 10);
  if( idx == 0 ) {
    heart_color += 27;
  }
  for( int i = 0; i < NUM_LEDS; i++) {
    leds[i] = CHSV(heart_color, 255, 255 * ((heart[idx] >> (NUM_LEDS - 1 - i)) & 0x0001));
  }  
  base_Color(CRGB(255,0,0));
}
