/*
double calc(double px, double py, double dx, double dy, double ox, double oy) {
  double x = px + (dx*ox);
  double y = py + (dy*oy);
  return ((cos(x) * sin(y)) * 0.5) + 0.5;
}

double hpx = 0.5, hpy = 0.5, hdx = PI/8, hdy = PI/8;
double spx = 0.5, spy = 0.5, sdx = 0.0, sdy = 0.0;
double vpx = 0.5, vpy = 0.5, vdx = 0.0, vdy = 0.0;
void test_loop() {
  //px += (random(10) - 3) / 1000.0;
  //py += (random(10) - 3) / 1000.0;
  hpx += hdx;  hpy += hdy;  hdx += (double)(random(9) - 4) * 0.005;  hdy += (double)(random(9) - 4) * 0.005;
  //if( hpx > PI ) hpx -= PI;  if( hpx < -PI ) hpx += PI;  
  if( hdx > PI/4 ) hdx = PI/4;  if( hdx < -PI/4 ) hdx = -PI/4;
  //if( hpy > PI ) hpy -= PI;  if( hpy < -PI ) hpy += PI;  
  if( hdy > PI/4 ) hdy = PI/4;  if( hdy < -PI/4 ) hdy = -PI/4;
  spx += sdx;  spy += sdy;  sdx += (double)(random(9) - 4) * 0.002;  sdy += (double)(random(9) - 4) * 0.002;
  //if( spx > PI ) spx = PI;  if( spx < -PI ) spx = -PI;  
  if( sdx > PI/8 ) sdx = PI/8;  if( sdx < -PI/8 ) sdx = -PI/8;
  //if( spy > PI ) spy = PI;  if( spy < -PI ) spy = -PI;  
  if( sdy > PI/8 ) sdy = PI/8;  if( sdy < -PI/8 ) sdy = -PI/8;
  vpx += vdx;  vpy += vdy;  vdx += (double)(random(9) - 4) * 0.008;  vdy += (double)(random(9) - 4) * 0.008;
  //if( vpx > PI ) vpx = PI;  if( vpx < -PI ) vpx = -PI;  
  if( vdx > PI/8 ) vdx = PI/16;  if( vdx < -PI/16 ) vdx = -PI/16;
  //if( vpy > PI ) vpy = PI;  if( vpy < -PI ) vpy = -PI;  
  if( vdy > PI/8 ) vdy = PI/16;  if( vdy < -PI/16 ) vdy = -PI/16;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      leds[ XYsafe(x,y) ] = CHSV(
        calc(hpx,hpy,hdx,hdy,x,y) * 254,
        ((calc(spx,spy,sdx,sdy,x,y) * 0.5) + 0.5) * 254,
        ((calc(vpx,vpy,vdx,vdy,x,y) * 0.75) + 0.25) * 254
      );
    }
  } 
  FastLED.show(); 
  delay(10);
}
*/
void test_loop() {
  double step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    hue = step + (37 * sin( ((y*step)/(kMatrixHeight*PI)) * 0.04 ) );
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      hue += 17 * sin(x/(kMatrixWidth*PI));
      leds[ XYsafe(x,y) ] = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
  FastLED.show();
}
void calc_mic() {
  mic_cur = analogRead(A1);
  mic_delta = mic_cur - mic_old;
  mic_mean += mic_delta / 4;
  mic_mean_delta = mic_mean - mic_mean_old;
  
  mic_mean_far += mic_mean_delta / 16;
  mic_old = mic_cur;
  mic_mean_old = mic_mean;
}
void test2_loop() {
  //calc_mic();
  double step = 0x00400000 - ((millis() >> 6) & 0x003FFFFF); 
  byte hue = 0;
  for( byte y = 0; y < kMatrixWidth; y++ ) {
    hue = step + (89 * sin( ((y*step)/(kMatrixWidth*PI)) * 0.04 ) );
    for( byte x = 0; x < kMatrixHeight; x++ ) {
      hue += 61 * sin(x/(kMatrixHeight*PI));
      leds[ XYsafe(y,kMatrixHeight - 1 - x) ] = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.00829)), 255*sin((hue+step)*PI*0.003891));
    }
  }
  FastLED.show();
}


void test_setup() {
}
void test2_setup() {
}

