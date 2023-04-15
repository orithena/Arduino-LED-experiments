CRGB rainbowpalette(byte c) {
  c = c % 6;
  return CRGB(255*(c>2), 255*(((c+5)%6)<3), 255*(((c+1)%6)<3) );
}

void loop_RainbowSpiral(uint32_t ms) {
  for( byte i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = rainbowpalette((byte)(ms>>5) + ((i + ((byte)(ms>>4) & 0x01))>>1));
  }
  base_Color(CRGB(0,0,255));
}

CRGB rainbowpalette2(byte col) {
  byte c = col % 12;
  byte c2 = c>>1;
  byte r = 255*(c2>2)*(c&0x01);
  byte g = 255*(((c2+5)%6)<3)*(c&0x01);
  byte b = 255*(((c2+1)%6)<3)*(c&0x01);
  return CRGB(r,g,b);
}

void loop_RainbowSpiral2(uint32_t ms) {
  for( byte i = 0; i < NUM_LEDS; i++ ) {
    leds[NUM_LEDS-1-i] = rainbowpalette2((byte)(ms>>5) + ((i + ((byte)(ms>>4) & 0x01))>>1));
  }
  base_Color(CRGB(0,0,255));
}

void loop_Stars(uint32_t ms) {
  int r = random(NUM_LEDS);
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(random(255), 255, 255 * (r == i || r-(NUM_LEDS/2) == i));
  }
  delay(20);
  base_Color(CRGB(255,255,255));
}

void loop_Stars2(uint32_t ms) {
  fadeToBlackBy( leds, NUM_LEDS, 10 );
  int r = random(NUM_LEDS);
  leds[r] = CHSV(random(255), 255, 255);
  r = random(NUM_LEDS);
  leds[r] = CHSV(random(255), 255, 255);
  delay(20);
  base_Color(CRGB(255,255,255));
}

void loop_Red(uint32_t ms) {
  int r = random(2*NUM_LEDS);
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(0, 255, 255 * (r != i));
  }
  base_Color(CRGB(255,0,0));
  delay(20);
}

/*
void loop_Pong(uint32_t ms) {
  byte col = ((ms >> 5) & 0x000000FF);
  byte pos = ((ms >> 4) & 0x00000FFF) % NUM_LEDS;
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(col, 255, 255*(pos==i));
  }
}
*/
void loop_Pong(uint32_t ms) {
  byte col = ((ms >> 5) & 0x000000FF);
  byte pos1 = (NUM_LEDS/2) + (cos(((ms >> 1) & 0x0000FFFF) / (8*PI)) * (NUM_LEDS/2));
  byte pos2 = (NUM_LEDS/2) + (sin(((ms >> 1) & 0x0000FFFF) / (5*PI)) * (NUM_LEDS/2));
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(col, 255, 255*(pos1==i || pos2==i));
  }
  base_RGB(ms);
}

#define LISSAJOUS 4
void loop_Lissajou(uint32_t ms) {
  Clear();
  for( int i = 0; i < LISSAJOUS; i++ ) {
    byte col = ((ms >> 5) & 0xFF) + ((256/LISSAJOUS)*i); 
    byte pos = (NUM_LEDS/2) + (sin(((ms >> 1) & 0x0000FFFF) / ((5+i)*PI)) * (NUM_LEDS/2));
    pos = max(0, min(NUM_LEDS-1, pos));
    leds[pos] |= CHSV(col, 255, 255);
  }
}

void loop_FunnyStars(uint32_t ms) {
  byte col = random(255);// ((ms >> 5) & 0x000000FF);
  byte pos = cos(ldexp((double)(ms & 0x0000FFFF), 16)) * NUM_LEDS;
  for( int i = 0; i < NUM_LEDS; i++ ) {
    leds[i] = CHSV(col, 255, 255*(pos==i));
  }
}

void loop_RandomStars(uint32_t ms) {
  Clear();
  byte col = random(255);
  byte count = random(NUM_LEDS/2)+(NUM_LEDS/4);
  for( int i = 0; i < count; i++ ) {
    leds[random(NUM_LEDS)] |= CHSV(col, 255, 255);
  }
}

void loop_RGBBlink(uint32_t ms) { 
  int col = (((ms >> 1) & 0x0000000F) / 5) * 85;
  for( int i = 0; i < NUM_LEDS; i++ ) {
    
    leds[i] = CHSV(col,255,255);

  }
  base_ColorBlink(ms, CRGB(255,255,255));
}

void loop_WhiteBlink(uint32_t ms) { 
  for( int i = 0; i < NUM_LEDS; i++ ) {
    
    leds[i] = CHSV(60,0,160 * ((ms >> 6) % 2));

  }
  
}

void loop_Rainbow2(uint32_t ms) { 
  Clear();
  int col = (ms>>2);
  for( int i = 0; i < NUM_LEDS/2; i++ ) {
    
    leds[i << 1] = CHSV(col + i*(256/NUM_LEDS),255,255);
    leds[(i << 1)+1] = CHSV(col + (NUM_LEDS - i)*(256/NUM_LEDS),255,255);

  }
  
}

void loop_Rainbow(uint32_t ms) { 
  Clear();
  int col = (ms >> 1);
  for( int i = 0; i < NUM_LEDS/2; i++ ) {
    
    leds[i] = CHSV(col + i*(128/NUM_LEDS),255,255);
    leds[NUM_LEDS - 1 - i] = CHSV(col + i*(128/NUM_LEDS),255,255);

  }
  base_RGB(ms);
}

void loop_Rainbow_Inc(uint32_t ms) { 
  Clear();
  byte lastled = ((ms >> 4) & 0x0FFF) % NUM_LEDS;
  byte offset = (((ms >> 6) & 0x00FF) % NUM_LEDS) * (255/NUM_LEDS);
  for( int i = 0; i <= lastled; i++ ) {
    
    leds[NUM_LEDS - 1 - i] = CHSV(offset + lastled + i*(255/NUM_LEDS),255,255);

  }
  base_RGB(ms);
}

void loop_Rainbow_Dec(uint32_t ms) { 
  Clear();
  byte lastled = ((ms >> 4) & 0x0FFF) % NUM_LEDS;
  byte offset = (((ms >> 6) & 0x00FF) % NUM_LEDS) * (255/NUM_LEDS);
  for( int i = 0; i <= lastled; i++ ) {
    
    leds[i] = CHSV(offset + lastled + i*(255/NUM_LEDS),255,255);

  }
  base_RGB(ms);
}

void loop_RGB_sawtooth(uint32_t ms) { 
  Clear();
  int count = (ms >> 3) % (NUM_LEDS/2);
  int col = ((ms >> 3) / (NUM_LEDS/2)) % 6;
  for( int i = 0; i <= count; i++ ) {
    
    leds[i] = CHSV(col*42,255,255);
    leds[NUM_LEDS - 1 - i] = CHSV(col*42,255,255);

  }
  base_RGB(ms);
}

void loop_RGB_sawtooth_inv(uint32_t ms) { 
  Clear();
  int count = (NUM_LEDS/2) - 1 - ((ms >> 3) % (NUM_LEDS/2));
  int col = ((ms >> 3) / (NUM_LEDS/2)) % 6;
  for( int i = 0; i <= count; i++ ) {
    
    leds[(NUM_LEDS/2) + i] = CHSV(col*42,255,255);
    leds[(NUM_LEDS/2) - 1 - i] = CHSV(col*42,255,255);

  }
  base_RGB(ms);
}
