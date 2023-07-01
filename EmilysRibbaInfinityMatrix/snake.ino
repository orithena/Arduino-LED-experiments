typedef struct IntMatrix {
  int a11;
  int a12;
  int a21;
  int a22;
} IntMatrix;

IntMatrix snake[] = {
  { .a11 = kMatrixWidth*2/3, .a12 = kMatrixHeight*2/3, .a21 =  1,  .a22 =  0 },
  { .a11 = kMatrixWidth/3,   .a12 = kMatrixHeight*2/3, .a21 =  0,  .a22 =  1 },
  { .a11 = kMatrixWidth*2/3, .a12 = kMatrixHeight/3,   .a21 = -1,  .a22 =  0 },
  { .a11 = kMatrixWidth/3,   .a12 = kMatrixHeight/3,   .a21 =  0,  .a22 = -1 },  
  { .a11 = kMatrixWidth*2/3, .a12 = kMatrixHeight/2,   .a21 = -1,  .a22 =  0 },
  { .a11 = kMatrixWidth/3,   .a12 = kMatrixHeight/1,   .a21 =  0,  .a22 = -1 }  
};

Matrix dsnake[] = {
  { .a11 = kMatrixWidth*2/3, .a12 = kMatrixHeight*2/3, .a21 =  1,  .a22 =  0 },
  { .a11 = kMatrixWidth/3,   .a12 = kMatrixHeight*2/3, .a21 =  0,  .a22 =  1 },
  { .a11 = kMatrixWidth*2/3, .a12 = kMatrixHeight/3,   .a21 = -1,  .a22 =  0 },
  { .a11 = kMatrixWidth/3,   .a12 = kMatrixHeight/3,   .a21 =  0,  .a22 = -1 },  
  { .a11 = kMatrixWidth*2/3, .a12 = kMatrixHeight/2,   .a21 = -1,  .a22 =  0 },
  { .a11 = kMatrixWidth/3,   .a12 = kMatrixHeight/1,   .a21 =  0,  .a22 = -1 }  
};

void fadeToBlackLog() {
  for( int i = 0; i < NUM_LEDS; i++ ) {
    double f = rgb2hsv_approximate(leds(i)).v/200.0;
    uint16_t x = f*leds(i).r;
    leds(i).r = leds(i).r == 0 ? 0 : x < leds(i).r ? x : leds(i).r-1;
    x = f*leds(i).g;
    leds(i).g = leds(i).g == 0 ? 0 : x < leds(i).g ? x : leds(i).g-1;
    x = f*leds(i).b;
    leds(i).b = leds(i).b == 0 ? 0 : x < leds(i).b ? x : leds(i).b-1;
  }
}

bool is_empty(int x, int y) {
  if( x < 0 || x >= kMatrixWidth || y < 0 || y >= kMatrixHeight ) {
    return false;
  }
  return ( leds(x,y).r == 0 && leds(x,y).g == 0 && leds(x,y).b == 0 );
}

bool move_snake_if_empty(int i, int dx, int dy) {
  int n1 = snake[i].a11 + dx;
  int n2 = snake[i].a12 + dy;
  if( is_empty(n1, n2) ) {
    snake[i].a11 = n1;
    snake[i].a12 = n2;
    return true;
  } else {
    return false;
  }
}

void rotate_snake(int i) {
  int t = snake[i].a22;
  snake[i].a22 = snake[i].a21;
  snake[i].a21 = t;
  if( random(2) == 0 ) {
    snake[i].a21 *= (-1);
    snake[i].a22 *= (-1);
  }
}

void print_snakes() {
  for( int i = 0; i < ARRAY_SIZE(snake); i++ ) {
    Serial.printf("%d: .a11=%2d  .a12=%2d  .a21=%2d  .a22=%2d\n", i, snake[i].a11, snake[i].a12, snake[i].a21, snake[i].a22); 
  }
  Serial.println();
}

void snakes() {
  fadeToBlackLog();
  EVERY_N_MILLISECONDS(100) {
    for( int i; i < ARRAY_SIZE(snake); i++ ) {
      bool has_moved = false;
      int attempts = 0;
      while( !has_moved && attempts < 8 ) {
        has_moved = move_snake_if_empty(i, snake[i].a21, snake[i].a22);
        if( !has_moved ) {
          rotate_snake(i);
        }
        attempts++;
      }
      if( random(16) == 0 ) {
        rotate_snake(i);
      }
      // chase color with uniform speed
      // leds(snake[i.a11, snake[i].a12)] = CHSV(gHue[0]+(i*(255/ARRAY_SIZE(snake))), 255, 255);

      // chase color with changing speed
      double hue_d = (sin(millis()/8121.0)*gHue[0]) + (sin(millis()/7177.0)*i*(255/ARRAY_SIZE(snake)));
      uint8_t hue = hue_d;
      leds(snake[i].a11, snake[i].a12) = CHSV(hue, 255, 255);
      
    }
  }
}

bool double_move_snake_if_empty(int i, double dx, double dy) {
  double n1 = dsnake[i].a11 + dx;
  double n2 = dsnake[i].a12 + dy;
  if( ((int)(n1) == (int)(dsnake[i].a11) && (int)(n2) == (int)(dsnake[i].a12)) || is_empty(n1, n2) ) {
    dsnake[i].a11 = n1;
    dsnake[i].a12 = n2;
    return true;
  } else {
    return false;
  }
}

void double_rotate_snake(int i) {
  double temp   = dsnake[i].a22;
  dsnake[i].a22 = dsnake[i].a21;
  dsnake[i].a21 = temp;
  if( random(2) == 0 ) {
    dsnake[i].a21 *= (-1);
    dsnake[i].a22 *= (-1);
  }
}

void print_dsnakes() {
  for( int i = 0; i < ARRAY_SIZE(snake); i++ ) {
    Serial.printf("%d: .a11=%6.3f  .a12=%6.3f  .a21=%6.3f  .a22=%6.3f\n", i, dsnake[i].a11, dsnake[i].a12, dsnake[i].a21, dsnake[i].a22); 
  }
  Serial.println();
}

void doublesnakes() {
  fadeToBlackLog();
  EVERY_N_MILLISECONDS(20) {
    for( int i; i < ARRAY_SIZE(snake); i++ ) {
      bool has_moved = false;
      double speed_factor = ( sin( ( (((millis() >> 3) & 0xFF) / 255.0) + (i / 8.0) ) * 2 * PI ) / 12.0 ) + 0.15; 
      int attempts = 0;
      while( !has_moved && attempts < 8 ) {
        has_moved = double_move_snake_if_empty(i, speed_factor * dsnake[i].a21, speed_factor * dsnake[i].a22);
        if( !has_moved ) {
          double_rotate_snake(i);
        }
        attempts++;
      }
      if( random(24) == 0 ) {
        double_rotate_snake(i);
      }

      double hue_d = (sin(millis()/8121.0)*gHue[0]) + (sin(millis()/7177.0)*i*(255/ARRAY_SIZE(dsnake)));
      uint8_t hue = hue_d;
      leds(dsnake[i].a11, dsnake[i].a12) = CHSV(hue, 255, 255);
    }
  }
}
