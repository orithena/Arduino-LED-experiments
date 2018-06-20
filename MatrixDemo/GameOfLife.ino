#define GOL_ROUNDTIME 768
#define GOL_MAX_REPETITIONS 5

#define GOL_LAST gol_stat.current_buf^0x01
#define GOL_CUR gol_stat.current_buf
#define GOL_HARE1 2
#define GOL_HARE2 3
#define GOL_LAST_BUF gol_stat_buf[GOL_LAST]
#define GOL_CUR_BUF gol_stat_buf[GOL_CUR]
#define GOL_HARE1_BUF gol_stat_buf[GOL_HARE1]
#define GOL_HARE2_BUF gol_stat_buf[GOL_HARE2]

byte gol_stat_buf[ 4 ][ kMatrixWidth ][ kMatrixHeight ];

typedef struct GOL_InternalStatus {
  byte current_buf;
  int iterations;
  uint32_t nextrun;
  int fadestep;
  int repetitions;
} GOL_InternalStatus;

GOL_InternalStatus gol_stat = {
  current_buf : 0,
  iterations : 0,
  nextrun : 0,
  fadestep : 0,
  repetitions : 0
};

void gol_loop()
{
  uint32_t ms = millis();

  if( gol_stat.nextrun - ms > GOL_ROUNDTIME ) {
    gol_stat.nextrun = ms;
  }

  if( ms >= gol_stat.nextrun ) {
    if( gol_stat.repetitions > GOL_MAX_REPETITIONS ) {
      gol_randomize_buffers();
    } 
    else {
      gol_generation_control();
    }
    gol_stat.nextrun = ms + GOL_ROUNDTIME; 
  }
  gol_fader( GOL_ROUNDTIME-(gol_stat.nextrun - ms) );

}

int _mod(int x, int m) {
  while( x >= m ) x -= m;
  while( x <  0 ) x += m;
  return x;
}

byte gol_valueof(byte b, int x, int y) {
  return gol_stat_buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)] > 0 ? 1 : 0;
}

boolean gol_alive(byte b, int x, int y) {
  return (gol_valueof(b,x,y) & 0x01) > 0;
}

byte gol_colorof(byte b, int x, int y) {
  return gol_stat_buf[b][_mod(x,kMatrixWidth)][_mod(y,kMatrixHeight)];
}

int gol_neighborsalive(byte b, int x, int y) {
  int count = 0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        int v = gol_valueof(b,xi,yi);
        count += v;
      }
    }
  }
  return count;
}

byte gol_meanneighborcolor(byte b, int x, int y) {
  double cos_sum = 0.0;
  double sin_sum = 0.0;
  for( int xi = x-1; xi <= x+1; xi++ ) {
    for( int yi = y-1; yi <= y+1; yi++ ) {
      if( !(xi == x && yi == y)) {
        if( gol_valueof(b,xi,yi) > 0 ) {
          double phi = (((double)(gol_colorof(b,xi,yi)) * PI) / 128.0);
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

void gol_randomize_buffers() {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      GOL_LAST_BUF[x][y] = 0;
      GOL_HARE2_BUF[x][y] = 0;
      byte r;
      if( random(2) == 0) {
        r = 0;
      } else {
        r = (random(3)*85 ) + 1;
      }
      GOL_CUR_BUF[x][y] = r;
      GOL_HARE1_BUF[x][y] = r;
    }
  }
  gol_stat.iterations = 0;
  gol_stat.repetitions = 0;
}

boolean gol_buf_compare(int b1, int b2) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      if( (gol_stat_buf[b1][x][y] == 0) != (gol_stat_buf[b2][x][y] == 0) ) {
        return false;
      }
    }
  }
  return true;
}

int gol_generation(int from, int to) {
  int currentlyalive = 0;
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      int n = gol_neighborsalive(from, x, y);
      if( gol_alive(from,x,y) ) {
        if( n < 2 ) { //underpopulation
          gol_stat_buf[to][x][y] = 0;
        } 
        else if( n > 3 ) { //overpopulation
          gol_stat_buf[to][x][y] = 0;
        } 
        else { //two or three neighbors
          gol_stat_buf[to][x][y] = gol_stat_buf[from][x][y];
          currentlyalive++;
        }
      } 
      else {
        if( n == 3 ) { //reproduction
          gol_stat_buf[to][x][y] = gol_meanneighborcolor(from, x, y);
          currentlyalive++;
        } 
        else {
          gol_stat_buf[to][x][y] = 0;
        }
      }
    }
  }
  return currentlyalive;
}

void gol_generation_control() {
  GOL_CUR = GOL_LAST;
  
  // this is the hare. it always computes two generations. It's never displayed.
  gol_generation(GOL_HARE1, GOL_HARE2);
  gol_generation(GOL_HARE2, GOL_HARE1);
  
  // this is the tortoise. it always computes one generation. The tortoise is displayed.
  gol_generation(GOL_LAST, GOL_CUR);
  
  // if hare and tortoise meet, the hare just went ahead of the tortoise through a repetition loop.
  if( gol_buf_compare(GOL_CUR, GOL_HARE1) ) {
    gol_stat.repetitions++;
  }

  gol_stat.iterations++;
}

void gol_fader(int cstep) {
  int spd = GOL_ROUNDTIME / 2;
  if( cstep >= 0  ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < kMatrixHeight; y++ ) {
        byte from = gol_valueof(GOL_LAST, x, y);
        byte to = gol_valueof(GOL_CUR, x, y);
        leds[ x + (kMatrixWidth*y) ] = CHSV(
          gol_colorof(to == 0 ? GOL_LAST : GOL_CUR, x, y), 
          255, 
          (byte)((double)(from*255) + ((to*255)-(from*255)) * min(1.0, ((double)cstep) / (double)spd))
        ); 
      }
    }
  }
}

