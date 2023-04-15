#include <Arduino.h>
#include <SNESpaduino.h>

#define PRELLBOCKZEIT        50            // buttons cannot be counted as pressed twice in this time frame  
#define WIEDERHOLUNGSZEIT   125

#define SLOW_MOVE_TIME  1500
#define SLOW_MOVE_STEP   150
#define FAST_MOVE_TIME    75
#define DEMO_TIME_SPEEDUP 3
#define LEVEL_FORMULA lines / 12

// Params for width and height of the game board
const uint8_t kBoardWidth = 14;
const uint8_t kBoardHeight = 14;

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
const int8_t stones[7][4][4][2] = { 
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
int hpos = 0;
int stone_x = kBoardWidth/2;
int stone_y = kBoardHeight;
int lines = 0;
int points = 0;
int level = 0;
volatile uint32_t oldmillis = 0;
uint32_t last_stone_move = 0;

int air_below(int stone_idx, int pos_x, int pos_y, int rotation) {
  int count = _max(_max(_max( 2 - pos_x, pos_x - (kBoardWidth-1) + 2), 0), 2);
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
    int when = _max(repeat/6, 10);
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
        DrawNumberOneFrame( points, (n - when)/2, 4);
      }
      FastLED.show();
      delay(33);
    }
  }
}

void rotate(bool left) {
  int test_rot = (rotation + 1) % 4;
  if( left ) {
    test_rot = (rotation + 3) % 4;
  }
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
      //else if( x >= 5 && x < 10 && (Char(Font5x7, 48+level)[x-5] >> (12 - y) & 1) * 255 ) {
      //  // show level number
      //  leds[ XYsafe(x,y) ] = CHSV(0,0,0);
      //}
      else {
        //leds[ XYsafe(x,y) ] = CHSV(170-(((y+board_offset)*170)/kBoardHeight), 255, 32 + (((y+board_offset)%5)*8) );
        leds[ XYsafe(x,y) ] = CHSV(0, 255, 32 );
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

uint32_t last_right = 0;
uint32_t last_left = 0;
uint32_t last_down = 0;
uint32_t last_A = 0;
uint32_t last_B = 0;
bool pull_down = false;
uint16_t last_btns = 0;

void readController() {
  uint32_t ms = millis();

  uint16_t btns = (pad.getButtons(true) & 0b0000111111111111);
  
  if( btns & BTN_RIGHT && (((last_btns & BTN_RIGHT) == 0 && ms - last_right > PRELLBOCKZEIT) || ms - last_right > WIEDERHOLUNGSZEIT) ) {
    if( hpos < kBoardWidth - 1 ) hpos++;
    last_right = ms;
    Serial.println("RIGHT");
  }
  if( btns & BTN_LEFT && (((last_btns & BTN_LEFT) == 0 && ms - last_left > PRELLBOCKZEIT) || ms - last_left > WIEDERHOLUNGSZEIT) ) {
    if( hpos > 0 ) hpos--;
    last_left = ms;
    Serial.println("LEFT");
  }
  if( btns & BTN_B && (((last_btns & BTN_B) == 0 && ms - last_B > PRELLBOCKZEIT) || ms - last_B > WIEDERHOLUNGSZEIT) ) {
    rotate(true);
    last_B = ms;
    Serial.println("Button B");
  }
  if( btns & BTN_A && (((last_btns & BTN_A) == 0 && ms - last_A > PRELLBOCKZEIT) || ms - last_A > WIEDERHOLUNGSZEIT) ) {
    rotate(false);
    last_A = ms;
    Serial.println("Button A");
  }
  if( btns & BTN_START ) {
    tetris_demo_mode = false;
    Serial.println("Start");
  }
  pull_down = (btns & BTN_DOWN);
  last_btns = btns;
}

int index_tr(int idx, int highest) {
  return (idx % 2 == 0 ? idx/2 : highest - idx/2);
}

void tetris_loop()
{
  uint32_t ms = millis();
  readController();

  int board_offset = 0; //((analog_left * (kBoardHeight - kMatrixHeight)) / 240);
  if( tetris_demo_mode ) {
    //set scroll position
    board_offset = stone_y - 4;
    if(board_offset < 0) {
      board_offset = 0;  
    }
    if(board_offset > kBoardHeight - kMatrixHeight) {
      board_offset = kBoardHeight - kMatrixHeight;
    }
  }

//  Serial.printf("hpos is: %d\n" + hpos);

  //int new_x = kBoardWidth - 1 - ((hpos * kBoardWidth) / 256);
  int new_x = hpos;

  if( tetris_demo_mode ) {
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
      rotate(false);
    }
  }

  for( int x = stone_x; (stone_x < new_x ? x <= new_x : x >= new_x); x = (stone_x < new_x ? x+1 : x-1) ) {
    if( possible(x, stone_y, rotation) ) {
      stone_x = x;
      hpos = x;
    } 
    else {
      break;
    }
  }

  if( ms - last_stone_move > (pull_down ? FAST_MOVE_TIME : (SLOW_MOVE_TIME - (level*SLOW_MOVE_STEP) )/(tetris_demo_mode ? DEMO_TIME_SPEEDUP : 1) ) ) {
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
      if( tetris_demo_mode ) {
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
      //stone_x = kBoardWidth - 1 - ((hpos * kBoardWidth) / 256);
      stone_x = hpos;
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
          tetris_demo_mode = true;
        }
        for( int y = kBoardHeight - 1; y >= 0; y-- ) {
          destroy_line(y);
        }
        for( int y = kMatrixHeight - 1; y >= 0; y-- ) {
          if( (y > kMatrixHeight - 3) || pad.getButtons(true) & BTN_DOWN ) {
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
//  FastLED.show();
#ifdef DEBUG
//  delay(20);
#else
//  delay(1);
#endif
}

void tetris_setup() {
//  tetris_demo_mode = true;
  randomSeed(micros());
}


