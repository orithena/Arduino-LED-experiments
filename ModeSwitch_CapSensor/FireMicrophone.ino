
byte buf[ 2 ][ kMatrixWidth+2 ][ kMatrixHeight+4 ];
byte bufi = 0;
int mic_cur, mic_old, mic_delta, mic_mean, mic_mean_delta, mic_mean_old, mic_mean_far = 0;


void firemic_loop()
{
  mic_cur = analogRead(A1);
  mic_delta = mic_cur - mic_old;
  mic_mean += mic_delta / 4;
  mic_mean_delta = mic_mean - mic_mean_old;
  
  
  
  mic_mean_far += mic_mean_delta / 16;
  mic_old = mic_cur;
  mic_mean_old = mic_mean;

  uint32_t ms = millis();
  ms = ms & 0x0003FFFF;

  FireLoop();
  //MicroShow(ms);
  FastLED.show();
}

void MicroShow(uint32_t ms) {
  //if(mic_delta > 8) { leds[ XYsafe(0,0) ] = CRGB(0,0,255); }
  for( byte x = kMatrixWidth - 1; x > 0; x-- ) {
    for( byte y = 0; y < kMatrixHeight; y++ ) {
      leds[ XYsafe(x,y) ] = leds[ XYsafe(x-1, y) ];
    }
  }
  int l = (abs(mic_mean_far - mic_cur)*8);
  for( int i = 0; i < kMatrixHeight; i++) {
    leds[ XYsafe(0,i) ] = CRGB(0,0, min(255, l) );
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

CRGB firepalette(byte shade) {
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
      byte r = random(128);
      buf[bufi][x][kMatrixHeight+2] = r+48;
      buf[bufi][x][kMatrixHeight+3] = r+96;
    }
  }
  if( mic_mean_delta > 6 ) {
    byte width = min(4, mic_mean_delta/8.) + 1;
    byte shade = max(0, 64 - (mic_mean_delta/width));
    byte offset = 1+width+random(kMatrixWidth-width-1);
    for( byte x = offset-width; x <= offset+width; x++) {
      for( byte y = 1; y <= 3; y++)  {
        buf[bufi][x][kMatrixHeight+y] = 255-((y-1)*16)-shade;
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
      leds[ XYsafe(x, y) ] = firepalette( buf[bufi][x+1][kMatrixHeight - y - 1] );
    }
  }
  firecount++;
}

void RainbowLoop(uint32_t ms) {
  int32_t yHueDelta32 = ((int32_t)cos16( ms * (27 / 1) ) * (350 / kMatrixWidth));
  int32_t xHueDelta32 = ((int32_t)cos16( ms * (39 / 1) ) * (310 / kMatrixHeight));
  DrawRainbowOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
}

void PaletteTestLoop(uint32_t ms) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      leds[ XYsafe(x,y) ] = firepalette( ((ms/100) + (x*kMatrixWidth) + y) % 255 );
    }
  }
}


void DrawRainbowOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8) {
  byte lineStartHue = startHue8;
  for ( byte y = 0; y < kMatrixHeight; y++) {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;
    for ( byte x = 0; x < kMatrixWidth; x++) {
      pixelHue += xHueDelta8;
      leds[ XY(x, y)]  = CHSV( pixelHue, 255, 255);
    }
  }
}

void firemic_setup() {
  pinMode(A1, INPUT);
  mic_mean_far = mic_mean = mic_old = mic_mean_old = analogRead(A1);
}




