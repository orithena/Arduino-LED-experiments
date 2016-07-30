#define GOL_ROUNDTIME 512

byte gol_buf[ 4 ][ kMatrixWidth ][ kMatrixHeight ];
byte gol_bufi = 0;

int currentcolor = 0;
int currentlyalive = 0;
int iterations = 0;
uint32_t nextrun = 0;
int fadestep = 0;
int repetitions = 0;

void gol_loop()
{
  uint32_t ms = millis();

  if( ms > nextrun ) {
    if( repetitions > 5 ) {
      GameOfLifeInit();
    } 
    else {
      GameOfLifeLoop();
    }
    nextrun = ms + GOL_ROUNDTIME; 
  }
  GameOfLifeFader( GOL_ROUNDTIME-(nextrun - ms) );
  FastLED.show();
}

byte valueof(byte b, int x, int y) {
  return gol_buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)] > 0 ? 1 : 0;
}

byte colorof(byte b, int x, int y) {
  return gol_buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)];
}

int neighbors(byte b, int x, int y) {
  int count = 0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        int v = valueof(b,xi,yi);
        count += v;
      }
    }
  }
  return count;
}

byte neighborcolor(byte b, int x, int y) {
  double cos_sum = 0.0;
  double sin_sum = 0.0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        if( valueof(b,xi,yi) > 0 ) {
          double phi = (((double)(colorof(b,xi,yi)) * PI) / 128.0);
          cos_sum = cos_sum + cos(phi);
          sin_sum = sin_sum + sin(phi);
        }
      }
    }
  }
  double phi_n = atan2(sin_sum, cos_sum);
  while( phi_n < 0.0 ) phi_n += 2*PI;
  byte color = (phi_n * 128.0) / PI;
  if(color == 0) color = 1;
  return color;
}

boolean alive(byte b, int x, int y) {
  return (valueof(b,x,y) & 0x01) > 0;
}

void printBoard(int b) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      Serial.print(valueof(b, x, y));
      Serial.print(neighbors(b, x, y));
      Serial.print("\t");
    }
    Serial.println();
  }
  Serial.println();
}

#ifdef DEBUG
static unsigned char gol[] = {
  //  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
    //  0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0x00
};
#endif

void GameOfLifeInit() {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      gol_buf[gol_bufi^0x01][x][y] = 0;
      gol_buf[3][x][y] = 0;
#ifdef DEBUG
      gol_buf[gol_bufi][x][y] = (gol[x] >> y) & 0x01;
      gol_buf[2][x][y] = (gol[x] >> y) & 0x01;
#else
      byte r;
      if( random(2) == 0) {
        r = 0;
      } else {
        r = (random(3)*85 ) + 1;
      }
      gol_buf[gol_bufi][x][y] = r;
      gol_buf[2][x][y] = r;
#endif
    }
  }
#ifdef DEBUG
  printBoard(gol_bufi);
#endif
  currentcolor = random(256);
  iterations = 0;
  currentlyalive = 10;
  repetitions = 0;
}

boolean gol_buf_compare(int b1, int b2) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      if( (gol_buf[b1][x][y] == 0) != (gol_buf[b2][x][y] == 0) ) {
        return false;
      }
    }
  }
  return true;
}

int generation(int from, int to) {
  int currentlyalive = 0;
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      int n = neighbors(from, x, y);
      if( alive(from,x,y) ) {
        if( n < 2 ) { //underpopulation
          gol_buf[to][x][y] = 0;
        } 
        else if( n > 3 ) { //overpopulation
          gol_buf[to][x][y] = 0;
        } 
        else { //two or three neighbors
          gol_buf[to][x][y] = gol_buf[from][x][y];
          currentlyalive++;
        }
      } 
      else {
        if( n == 3 ) { //reproduction
          gol_buf[to][x][y] = neighborcolor(from, x, y);
          currentlyalive++;
        } 
        else {
          gol_buf[to][x][y] = 0;
        }
      }
    }
  }
  return currentlyalive;
}

void GameOfLifeLoop() {
  gol_bufi = gol_bufi ^ 0x01;
  
  // this is the hare. it always computes two generations. It's never displayed.
  int dummy = generation(2,3);
  dummy = generation(3,2);
  
  // this is the tortoise. it always computes one generation. The tortoise is displayed.
  currentlyalive = generation(gol_bufi^0x01, gol_bufi);
  
  // if hare and tortoise meet, the hare just went ahead of the tortoise through a repetition loop.
  if( gol_buf_compare(gol_bufi, 2) ) {
    repetitions++;
  }

  iterations++;
}

void GameOfLifeFader(int cstep) {
  int spd = (GOL_ROUNDTIME * 3) / 4;
  if( cstep <= spd ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        double from = valueof(gol_bufi^0x01, x, y);
        double to = valueof(gol_bufi, x, y);
        leds[ XYsafe(x,y) ] = CHSV(colorof(to == 0 ? gol_bufi^0x01 : gol_bufi, x, y), 255, 255 * (from + ((to-from) * cstep) / spd) );
      }
    }
  }
}

void gol_setup() {
}


