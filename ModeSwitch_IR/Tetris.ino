#include <Arduino.h>

#define IN_ANALOGRIGHT_PIN   A3
#define IN_ANALOGLEFT_PIN    A4
#define IN_ANALOGLEFT_LOWER_BOUND    298   // min raw value of left analog controller
#define IN_ANALOGLEFT_UPPER_BOUND    744   // max raw value of left analog controller
#define IN_ANALOGRIGHT_LOWER_BOUND   235   // min raw value of right analog controller
#define IN_ANALOGRIGHT_UPPER_BOUND   678   // max raw value of right analog controller
#define ANALOG_JITTER         16           // not counting analog changes below that threshold
#define ANALOG_JITTER_FIR_ORDER   4.0      // jitter smoothing filter length

#define IN_BUTTONLEFT_PIN     4            // left digital button pin
#define IN_BUTTONRIGHT_PIN    3            // right digital button pin
#define PRELLBOCKZEIT       168            // buttons cannot be counted as pressed twice in this time frame  

#define SLOW_MOVE_TIME  1500
#define SLOW_MOVE_STEP   150
#define FAST_MOVE_TIME    75
#define DEMO_TIME_SPEEDUP 3
#define LEVEL_FORMULA lines / 12

// Params for width and height of the game board
const uint8_t kBoardWidth = 10;
const uint8_t kBoardHeight = 16;

byte board[ kBoardWidth ][ kBoardHeight ];

#define _AA {-1,0}
#define _AB {-1,1}
#define _AC {-1,2}
#define _BA {0,0}
#define _BB {0,1}
#define _BC {0,2}
#define _BD {0,3}
#define _CA {1,0}
#define _CB {1,1}
#define _CC {1,2}
#define _DA {2,0}

// stone structure: stones[number][rotation][part][0 => x value;1 => y value]
const char stones[7][4][4][2] = { 
  { // 0 = square
    { 
      _BA, _BB, _CA, _CB                                         }
    ,
    { 
      _BA, _BB, _CA, _CB                                         }
    ,
    { 
      _BA, _BB, _CA, _CB                                         }
    ,
    { 
      _BA, _BB, _CA, _CB                                         }
  }
  , { // 1 = el block
    { 
      _AA, _AB, _BB, _CB                                         }
    ,
    { 
      _AC, _BA, _BB, _BC                                         }
    ,
    { 
      _AA, _BA, _CA, _CB                                         }
    ,
    { 
      _BA, _BB, _BC, _CA                                         }
  }
  , { // 2 = reverse squiggely
    { 
      _AB, _BB, _BA, _CA                                         }
    ,
    { 
      _AA, _AB, _BB, _BC                                         }
    ,
    { 
      _AB, _BB, _BA, _CA                                         }
    ,
    { 
      _AA, _AB, _BB, _BC                                         }
  }
  , { // 3 = reverse el block
    { 
      _AA, _AB, _BA, _CA                                         }
    ,
    { 
      _BA, _BB, _BC, _CC                                         }
    ,
    { 
      _AB, _BB, _CB, _CA                                         }
    ,
    { 
      _AA, _BA, _BB, _BC                                         }
  }
  , { // 4 = T block
    { 
      _AB, _BB, _BA, _CB                                         }
    ,
    { 
      _AB, _BB, _BA, _BC                                         }
    ,
    { 
      _AA, _BB, _BA, _CA                                         }
    ,
    { 
      _BA, _BB, _BC, _CB                                         }
  }
  , { // 5 = squiggely
    { 
      _AA, _BA, _BB, _CB                                         }
    ,
    { 
      _BC, _BB, _CB, _CA                                         }
    ,
    { 
      _AA, _BA, _BB, _CB                                         }
    ,
    { 
      _BC, _BB, _CB, _CA                                         }
  }
  , { // 6 = LINE PIECE!!!
    { 
      _BA, _BB, _BC, _BD                                         }
    ,
    { 
      _AA, _BA, _CA, _DA                                         }
    ,
    { 
      _BA, _BB, _BC, _BD                                         }
    ,
    { 
      _AA, _BA, _CA, _DA                                         }
  }
};

// internal state variables and their defaults
volatile byte stone = 6;
volatile byte rotation = 0;
volatile int analog_left, analog_left_raw_old = 0;
volatile int analog_right, analog_right_raw_old = 0;
int stone_x = kBoardWidth/2;
int stone_y = kBoardHeight;
int lines = 0;
int points = 0;
int level = 0;
volatile uint32_t oldmillis = 0;
uint32_t last_stone_move = 0;
volatile boolean demo_mode = true;

int air_below(int stone_idx, int pos_x, int pos_y, int rotation) {
  int count = max(max(max( 2 - pos_x, pos_x - (kBoardWidth-1) + 2), 0), 2);
  for( int p = 0; p < 4; p++ ) {
    if(stones[stone_idx][rotation][p][1] > 1) {
      count++;
      break;
    }
  }
  for( int p = 0; p < 4; p++ ) {
    int pixel_x = pos_x + stones[stone_idx][rotation][p][0];
    int pixel_y = pos_y + stones[stone_idx][rotation][p][1];
    boolean dontcount = false;
    for( int p2 = 0; p2 < 4; p2++ ) {
      if( pos_x + stones[stone_idx][rotation][p2][0] == pixel_x && pos_y + stones[stone_idx][rotation][p2][1] == pixel_y - 1 ) {
        dontcount = true;
      }
    }
    if( !dontcount ) {
      boolean below_surface = false;
      for( int y = pixel_y-1; y >= 0; y-- ) {
        if( board[pixel_x][y] == 0 ) {
          count += (below_surface ? 1 : 4);
        } else {
          below_surface = true;
        }
      }
    }
  }
  return count;
}



boolean possible_stone(int stone_idx, int pos_x, int pos_y, int rotation) {
  for( int p = 0; p < 4; p++ ) {
    int pixel_x = pos_x + stones[stone_idx][rotation][p][0];
    int pixel_y = pos_y + stones[stone_idx][rotation][p][1];
    if( !between(0, pixel_x, kBoardWidth-1 ) ) return false;
    if( pixel_y < 0 ) return false;
    if( between(0, pixel_x, kBoardWidth-1  ) &&
      between(0, pixel_y, kBoardHeight-1 ) ) {
      if( board[ pixel_x ][ pixel_y ] > 0 ) {
        return false;
      }
    }
  }
  return true;
}

boolean possible(int pos_x, int pos_y, int rotation) {
  return possible_stone(stone, pos_x, pos_y, rotation);
}

void blink_line(int y, int board_offset, int repeat) {
  int my = y-board_offset;
  if( my < kMatrixHeight ) {
    int when = max(repeat/6, 10);
    for( int n = 0; n < repeat; n++ ) {
      if( n >= when ) {
        Clear();
      }
      for( int x = 0; x < kMatrixWidth; x++ ) {
        if( my < 0 ) {
          leds[ XYsafe( x,0 ) ] = CHSV( 0, 0, ((n*kMatrixWidth)+(x*36)+(y*36L*(repeat%8)))%128 );
        } 
        else {
          leds[ XYsafe( x,my ) ] = CHSV( ((n*kMatrixWidth)+(x*36)+(y*36L*(repeat%8)))%256, 255, 128 );
        }
      }
      if( n >= when ) {
        DrawNumberOneFrame( points, (n - when)/2 );
      }
      FastLED.show();
      delay(33);
    }
  }
}

void rotate() {
  int test_rot = (rotation + 1) % 4;
  // lets try rotating that stone.
  if( possible( stone_x, stone_y, test_rot ) ) {
    rotation = test_rot;
  } // we might be at the right wall. try one position to the left.
  else if( possible( stone_x-1, stone_y, test_rot ) ) {
    stone_x -= 1;
  } // nope? might be the left wall. try one position to the right.
  else if( possible( stone_x+1, stone_y, test_rot ) ) {
    stone_x += 1;
  } // okay, we've still got the long stick that might want to move two positions to the left.
  else if( possible( stone_x-2, stone_y, test_rot ) ) {
    stone_x -= 2;
  } // I give up and leave it at that.
  else {
    test_rot = rotation;
  }
  rotation = test_rot;
}

void blank_line(int y, int board_offset, int pause) {
  int my = y-board_offset;
  if( my >= 0 && my < kMatrixHeight ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      leds[ XYsafe( x, my) ] = CHSV( 0,0,0 );
    }
    FastLED.show();
    delay(pause);
  }
}

void destroy_line(int y) {
  for( int dy = y; dy < kBoardHeight-1; dy++ ) {
    for( int dx = 0; dx < kBoardWidth; dx++ ) {
      board[dx][dy] = board[dx][dy+1];
    }
  }
  for( int dx = 0; dx < kBoardWidth; dx++ ) {
    board[dx][kBoardHeight-1] = 0;
  }
}

CHSV palette(byte colorindex) {
  return CHSV((((colorindex-1) * 36) + 36) % 256, 255, (colorindex > 0) ? 255 : 0);
}

void display(int board_offset) {
  boolean stone_displayed = false;
  for( int x = 0; x < kMatrixWidth; x++ ) {
    for( int y = 0; y < kMatrixHeight; y++ ) {
      byte to = board[x][y + board_offset];
      for( byte p = 0; p < 4; p++ ) {
        if( stone_x + stones[stone][rotation][p][0] == x && stone_y + stones[stone][rotation][p][1] == (y + board_offset)) {
          to = stone+1;
          stone_displayed = true;
        } 
      }
#ifdef DEBUG
      Serial.print(48+level);
      Serial.print(" ");
      Serial.print(x);
      Serial.print(" ");
      Serial.print(y);
      Serial.print(" ");
      Serial.print(Char(Font5x7, 48+level)[x], HEX);
      Serial.print(" ");
      Serial.print(Char(Font5x7, 48+level)[x], BIN);
      Serial.print(" ");
      Serial.println((Char(Font5x7, 48+level)[x] >> (6 - y) & 1), HEX);
#endif
      if( to > 0 ) {
        leds[ XYsafe(x,y) ] = palette(to);
      } 
      else if( x >= 3 && x < 8 && (Char(Font5x7, 48+level)[x-3] >> (6 - y) & 1) * 255 ) {
        leds[ XYsafe(x,y) ] = CHSV(0,0,0);
      }
      else {
        leds[ XYsafe(x,y) ] = CHSV(170-(((y+board_offset)*170)/kBoardHeight), 255, 48 + (((y+board_offset)%5)*8) );
      }
    }
  }
  if( !stone_displayed && (millis() % 300) < 96 ) {
    int base_line = (stone_y > board_offset) ? kMatrixHeight - 2 : 0;
    for( int x = 0; x < kMatrixWidth; x++ ) {
      for( int y = 0; y < 4; y++ ) {
        for( byte p = 0; p < 4; p++ ) {
          if( stone_x + stones[stone][rotation][p][0] == x && stones[stone][rotation][p][1] == y) {
            leds[ XYsafe(x,base_line + y) ] = CHSV(0, 0, 96);
          }
        }
      }
    }
  }
}

void readController() {
  int analog_right_raw = analog_right_raw_old + ((analogRead(IN_ANALOGRIGHT_PIN) - analog_right_raw_old) / ANALOG_JITTER_FIR_ORDER);
  int analog_left_raw = analog_left_raw_old + ((analogRead(IN_ANALOGLEFT_PIN) - analog_left_raw_old) / ANALOG_JITTER_FIR_ORDER);
  int analog_right_old = (((long)analog_right*(IN_ANALOGRIGHT_UPPER_BOUND - IN_ANALOGRIGHT_LOWER_BOUND))/255L) + IN_ANALOGRIGHT_LOWER_BOUND;
  int analog_left_old = (((long)analog_left*(IN_ANALOGLEFT_UPPER_BOUND - IN_ANALOGLEFT_LOWER_BOUND))/255L) + IN_ANALOGLEFT_LOWER_BOUND;
  if( abs(analog_left_old - analog_left_raw) > ANALOG_JITTER ) {
    if( analog_left_raw > IN_ANALOGLEFT_UPPER_BOUND ) analog_left_raw = IN_ANALOGLEFT_UPPER_BOUND;
    if( analog_left_raw < IN_ANALOGLEFT_LOWER_BOUND ) analog_left_raw = IN_ANALOGLEFT_LOWER_BOUND;
    analog_left = ((analog_left_raw - IN_ANALOGLEFT_LOWER_BOUND) * 255L) / (IN_ANALOGLEFT_UPPER_BOUND - IN_ANALOGLEFT_LOWER_BOUND);
    analog_left_raw_old = analog_left_raw;
#ifdef DEBUG
    Serial.print("left ");
    Serial.print(analog_left);
    Serial.print(" ");
    Serial.println(analog_left_raw_old);
#endif
  }
  if( abs(analog_right_old - analog_right_raw) > ANALOG_JITTER ) {
    if( analog_right_raw > IN_ANALOGRIGHT_UPPER_BOUND ) analog_right_raw = IN_ANALOGRIGHT_UPPER_BOUND;
    if( analog_right_raw < IN_ANALOGRIGHT_LOWER_BOUND ) analog_right_raw = IN_ANALOGRIGHT_LOWER_BOUND;
    analog_right = ((analog_right_raw - IN_ANALOGRIGHT_LOWER_BOUND) * 255L) / (IN_ANALOGRIGHT_UPPER_BOUND - IN_ANALOGRIGHT_LOWER_BOUND);
#ifdef DEBUG
    Serial.print("right ");
    Serial.print(analog_right);
    Serial.print(" ");
    Serial.println(analog_right_raw_old);
#endif
  }
  analog_left_raw_old = analog_left_raw;
  analog_right_raw_old = analog_right_raw;
}

int index_tr(int idx, int highest) {
  return (idx % 2 == 0 ? idx/2 : highest - idx/2);
}

void tetris_loop()
{
  uint32_t ms = millis();
  readController();

  int board_offset = ((analog_left * (kBoardHeight - kMatrixHeight)) / 240);
  if( demo_mode ) {
    //set scroll position
    board_offset = stone_y - 4;
    if(board_offset < 0) {
      board_offset = 0;  
    }
    if(board_offset > kBoardHeight - kMatrixHeight) {
      board_offset = kBoardHeight - kMatrixHeight;
    }
  }

  int new_x = kBoardWidth - 1 - ((analog_right * kBoardWidth) / 256);

  if( demo_mode ) {
    new_x = kBoardWidth/2;
    int new_r = 0;
    int best_air_below = 30000;
    for( int y = stone_y; y >= 0; y-- ) {
      for( int x = 0; x <= kBoardWidth-1; x++) {
        int tx = x; //alternative: tx = index_tr(x, kBoardWidth-1);
        for( int r = 0; r <= 3; r++) {
          int tr = index_tr(3-r, 3);
          if( possible_stone( stone,tx,y,tr ) && air_below( stone, tx, y, tr ) < best_air_below ) {
            boolean okay = true;
            for( int yy = y; yy < stone_y; yy++ ) {
              if( !possible( tx,yy,tr ) ) {
                okay = false;
              }
            }
            if( okay ) {
              new_x = tx;
              new_r = tr;
              best_air_below = air_below( stone, tx, y, tr );
            }
          }
        }
      }
      best_air_below += 1;
    }
    if( rotation != new_r ) {
      rotate();
    }
  }

  for( int x = stone_x; (stone_x < new_x ? x <= new_x : x >= new_x); x = (stone_x < new_x ? x+1 : x-1) ) {
    if( possible(x, stone_y, rotation) ) {
      stone_x = x;
    } 
    else {
      break;
    }
  }

  boolean button_left = (digitalRead(IN_BUTTONLEFT_PIN) == HIGH);
  if( ms - last_stone_move > (button_left ? FAST_MOVE_TIME : (SLOW_MOVE_TIME - (level*SLOW_MOVE_STEP) )/(demo_mode ? DEMO_TIME_SPEEDUP : 1) ) ) {
#ifdef DEBUG
    Serial.print("board_offset: ");
    Serial.println(board_offset);
    Serial.print("--> stone: ");
    Serial.print(stone);
    Serial.print(" stone_y: ");
    Serial.println(stone_y);
#endif
    int new_y = stone_y - 1;
    if( possible(stone_x, new_y, rotation) ) {
      stone_y = new_y;
      if( demo_mode ) {
        //set scroll position
        board_offset = stone_y - 4;
        if(board_offset < 0) {
          board_offset = 0;  
        }
        if(board_offset > kBoardHeight - kMatrixHeight) {
          board_offset = kBoardHeight - kMatrixHeight;
        }
      }
    } 
    else { 
      // stone cannot go down, fix it to board!
      points++;
      for( int p = 0; p < 4; p++ ) {
        if( between(0, stone_x + stones[stone][rotation][p][0], kBoardWidth-1) &&
          between(0, stone_y + stones[stone][rotation][p][1], kBoardHeight-1) ) {
          board[ stone_x + stones[stone][rotation][p][0] ][ stone_y + stones[stone][rotation][p][1] ] = stone + 1;
        }
      }

      // check for full lines
      int just_now_completed_lines = 0;
      for( int y = kBoardHeight-1; y >= 0; y-- ) {
        boolean line_full = true;
        for( int x = 0; x < kBoardWidth; x++ ) {
          if( board[x][y] == 0 ) line_full = false;
        }
        if( line_full ) {
          lines++;
          just_now_completed_lines++;
          points += 2 << just_now_completed_lines;
          blink_line(y, board_offset, (level != LEVEL_FORMULA ? 100 : 10));
          level = LEVEL_FORMULA;
          if( level > 10 ) {
            level = 10;
          }
          blank_line(y, board_offset, 100);
          destroy_line(y);

          // after that long animation, re-read the controller state
          readController();
        }
      }

      // new stone
      stone = random(7);
      rotation = 0;
      stone_x = kBoardWidth - 1 - ((analog_right * kBoardWidth) / 256);
      for( int p = 0; p < 4; p++ ) {
        while( stone_x + stones[stone][rotation][p][0] < 0 ) {
          stone_x++;
        }
        while( stone_x + stones[stone][rotation][p][0] >= kBoardWidth ) {
          stone_x--;
        }
      }
      stone_y = kBoardHeight-1;

      // check for game over!
      if( !possible(stone_x, stone_y, rotation) ) {
        if( points < 12 ) {
          demo_mode = true;
        }
        for( int y = kBoardHeight - 1; y >= 0; y-- ) {
          destroy_line(y);
        }
        for( int y = kMatrixHeight - 1; y >= 0; y-- ) {
          if( (y > kMatrixHeight - 3) || (digitalRead(IN_BUTTONLEFT_PIN) == LOW) ) {
            blink_line(y, 0, 90);
          }
          blank_line(y, 0, 0);
        }
        points = 0;
        level = 0;
        lines = 0;
      }
    }
    last_stone_move = ms;
#ifdef DEBUG
    Serial.print("<-- stone: ");
    Serial.print(stone);
    Serial.print(" stone_y: ");
    Serial.println(stone_y);
#endif
  }

  display(board_offset);
  /*
  uint32_t xms = ms / 8;
   int32_t yHueDelta32 = ((int32_t)cos16( xms * (27 / 1) ) * (350 / kMatrixWidth));
   int32_t xHueDelta32 = ((int32_t)cos16( xms * (39 / 1) ) * (310 / kMatrixHeight));
   DrawRainbowOneFrame( xms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
   */
  FastLED.show();
#ifdef DEBUG
  delay(20);
#else
  delay(1);
#endif
}

void button_left() {
  uint32_t ms = millis();
  // we need to filter out button chatter, so we only count the first impulse of a tight bunch.
  // define PRELLBOCKZEIT to match the chattering characteristics of your buttons. 
  // also, if the timer just moved the piece in that same timeframe, we don't count the button.
  if( ms > oldmillis+PRELLBOCKZEIT && ms - last_stone_move > PRELLBOCKZEIT) {
    last_stone_move = ms - SLOW_MOVE_TIME;
    oldmillis = ms;
  }
}

void button_right() {
  uint32_t ms = millis();
  if( ms > oldmillis+PRELLBOCKZEIT) {
    demo_mode = false;
    rotate();
    oldmillis = ms;
  }
}

void tetris_setup() {
  pinMode(IN_BUTTONLEFT_PIN, INPUT);
  pinMode(IN_BUTTONRIGHT_PIN, INPUT);
  //  attachInterrupt(IN_BUTTONLEFT_PIN-2, button_left, RISING);
  attachInterrupt(IN_BUTTONRIGHT_PIN-2, button_right, RISING);
#ifdef DEBUG
  Serial.begin(9600);
#endif
  randomSeed(analogRead(A2)+analogRead(IN_ANALOGRIGHT_PIN));
#ifdef DEBUG
  delay(3000);
#endif
}


















