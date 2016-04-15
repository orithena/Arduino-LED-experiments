#define GOL_ROUNDTIME 384

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

  //PaletteTestLoop(ms/10);
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

boolean alive(byte b, int x, int y) {
  return valueof(b,x,y) & 0x01 > 0;
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

static unsigned char gol[] = {
  //  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
  0x00, 0x00, 0x01, 0x05, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00
    //  0xFF, 0x00, 0xFF, 0x00, 0xFF, 0x0F, 0xFF, 0x00, 0xFF, 0x00
};

void GameOfLifeInit() {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      gol_buf[gol_bufi^0x01][x][y] = 0;
      gol_buf[3][x][y] = 0;
#ifdef DEBUG
      gol_buf[gol_bufi][x][y] = (gol[x] >> y) & 0x01;
      gol_buf[2][x][y] = (gol[x] >> y) & 0x01;
#else
      int r = random(2);
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
      if( gol_buf[b1][x][y] != gol_buf[b2][x][y] ) {
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
          gol_buf[to][x][y] = 1;
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

  /*  for( int x = 0; x < kMatrixWidth; x++ ) {
   for( int y = 0; y < kMatrixHeight; y++ ) {
   leds[ XYsafe(x,y) ] = CHSV(currentcolor, 255, 255 * gol_buf[gol_bufi][x][y]);
   }
   }*/
}

void GameOfLifeFader(int cstep) {
  if( cstep <= 192 ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        double from = gol_buf[gol_bufi^0x01][x][y];
        double to = gol_buf[gol_bufi][x][y];
        leds[ XYsafe(x,y) ] = CHSV(currentcolor, 255, 255 * (from + ((to-from) * cstep) / 192) );
      }
    }
  }
}

void gol_setup() {
}


