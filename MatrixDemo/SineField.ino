/*** sinefield demo pattern ***
 *  
 *  I don't know how this pattern works. I just experimented and it looked nice.
 */

void sinefield() {
  double step = (millis() >> 6) & 0x003FFFFF; 
  byte hue = 0;
  for( byte y = 0; y < kMatrixHeight; y++ ) {
    hue = step + (37 * sin( ((y*step)/(kMatrixHeight*PI)) * 0.04 ) );
    for( byte x = 0; x < kMatrixWidth; x++ ) {
      hue += 17 * sin(x/(kMatrixWidth*PI));
      leds[ (x*kMatrixWidth) + y ] = CHSV(hue + ((unsigned long)step & 0x000000FF), 192 - (63*cos((hue+step)*PI*0.004145)), 255*sin((hue+step)*PI*0.003891));
    }
  }
}
