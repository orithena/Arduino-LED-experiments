#include <Arduino.h>

#include <FastLED.h>

//#define DEBUG
#define LED_PIN               6            // LED chain data pin
#define COLOR_ORDER RGB                    // LED chip color order
#define CHIPSET     WS2811                 // LED chipset
#define BRIGHTNESS 128                     // LED overall brightness

#define IN_ANALOGRIGHT_PIN   A3
#define IN_ANALOGLEFT_PIN    A4
#define IN_ANALOGLEFT_LOWER_BOUND    298   // min raw value of left analog controller
#define IN_ANALOGLEFT_UPPER_BOUND    744   // max raw value of left analog controller
#define IN_ANALOGRIGHT_LOWER_BOUND   235   // min raw value of right analog controller
#define IN_ANALOGRIGHT_UPPER_BOUND   678   // max raw value of right analog controller
#define ANALOG_JITTER         16           // not counting analog changes below that threshold
#define ANALOG_JITTER_FIR_ORDER   4.0      // jitter smoothing filter length

#define IN_BUTTONLEFT_PIN     2            // left digital button pin
#define IN_BUTTONRIGHT_PIN    3            // right digital button pin
#define PRELLBOCKZEIT       168            // buttons cannot be counted as pressed twice in this time frame  

#define SLOW_MOVE_TIME  1500
#define SLOW_MOVE_STEP   150
#define FAST_MOVE_TIME    75
#define LEVEL_FORMULA lines / 12

// Params for width and height of the LED Matrix
const uint8_t kMatrixWidth = 10;
const uint8_t kMatrixHeight = 7;

// Params for width and height of the game board
const uint8_t kBoardWidth = 10;
const uint8_t kBoardHeight = 16;

// Param for different pixel layouts
const bool    kMatrixSerpentineLayout = true;
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if ( kMatrixSerpentineLayout == false) {
    i = (y * kMatrixWidth) + x;
  }

  if ( kMatrixSerpentineLayout == true) {
    if ( !(y & 0x01)) {
      // Odd rows run backwards
      uint8_t reverseX = (kMatrixWidth - 1) - x;
      i = (y * kMatrixWidth) + reverseX;
    } 
    else {
      // Even rows run forwards
      i = (y * kMatrixWidth) + x;
    }
  }

  return i;
}

#define NUM_LEDS (kMatrixWidth * kMatrixHeight)
CRGB leds_plus_safety_pixel[ NUM_LEDS + 1];
CRGB* leds( leds_plus_safety_pixel + 1);

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

static unsigned char Font5x7[] = {
  0x00, 0x00, 0x00, 0x00, 0x00,// (space)
  0x00, 0x00, 0x5F, 0x00, 0x00,// !
  0x00, 0x07, 0x00, 0x07, 0x00,// "
  0x14, 0x7F, 0x14, 0x7F, 0x14,// #
  0x24, 0x2A, 0x7F, 0x2A, 0x12,// $
  0x23, 0x13, 0x08, 0x64, 0x62,// %
  0x36, 0x49, 0x55, 0x22, 0x50,// &
  0x00, 0x05, 0x03, 0x00, 0x00,// '
  0x00, 0x1C, 0x22, 0x41, 0x00,// (
  0x00, 0x41, 0x22, 0x1C, 0x00,// )
  0x08, 0x2A, 0x1C, 0x2A, 0x08,// *
  0x08, 0x08, 0x3E, 0x08, 0x08,// +
  0x00, 0x50, 0x30, 0x00, 0x00,// ,
  0x08, 0x08, 0x08, 0x08, 0x08,// -
  0x00, 0x60, 0x60, 0x00, 0x00,// .
  0x20, 0x10, 0x08, 0x04, 0x02,// /
  0x3E, 0x51, 0x49, 0x45, 0x3E,// 0
  0x00, 0x42, 0x7F, 0x40, 0x00,// 1
  0x42, 0x61, 0x51, 0x49, 0x46,// 2
  0x21, 0x41, 0x45, 0x4B, 0x31,// 3
  0x18, 0x14, 0x12, 0x7F, 0x10,// 4
  0x27, 0x45, 0x45, 0x45, 0x39,// 5
  0x3C, 0x4A, 0x49, 0x49, 0x30,// 6
  0x01, 0x71, 0x09, 0x05, 0x03,// 7
  0x36, 0x49, 0x49, 0x49, 0x36,// 8
  0x06, 0x49, 0x49, 0x29, 0x1E,// 9
  0x00, 0x36, 0x36, 0x00, 0x00,// :
  0x00, 0x56, 0x36, 0x00, 0x00,// ;
  0x00, 0x08, 0x14, 0x22, 0x41,// <
  0x14, 0x14, 0x14, 0x14, 0x14,// =
  0x41, 0x22, 0x14, 0x08, 0x00,// >
  0x02, 0x01, 0x51, 0x09, 0x06,// ?
  0x32, 0x49, 0x79, 0x41, 0x3E,// @
  0x7E, 0x11, 0x11, 0x11, 0x7E,// A
  0x7F, 0x49, 0x49, 0x49, 0x36,// B
  0x3E, 0x41, 0x41, 0x41, 0x22,// C
  0x7F, 0x41, 0x41, 0x22, 0x1C,// D
  0x7F, 0x49, 0x49, 0x49, 0x41,// E
  0x7F, 0x09, 0x09, 0x01, 0x01,// F
  0x3E, 0x41, 0x41, 0x51, 0x32,// G
  0x7F, 0x08, 0x08, 0x08, 0x7F,// H
  0x00, 0x41, 0x7F, 0x41, 0x00,// I
  0x20, 0x40, 0x41, 0x3F, 0x01,// J
  0x7F, 0x08, 0x14, 0x22, 0x41,// K
  0x7F, 0x40, 0x40, 0x40, 0x40,// L
  0x7F, 0x02, 0x04, 0x02, 0x7F,// M
  0x7F, 0x04, 0x08, 0x10, 0x7F,// N
  0x3E, 0x41, 0x41, 0x41, 0x3E,// O
  0x7F, 0x09, 0x09, 0x09, 0x06,// P
  0x3E, 0x41, 0x51, 0x21, 0x5E,// Q
  0x7F, 0x09, 0x19, 0x29, 0x46,// R
  0x46, 0x49, 0x49, 0x49, 0x31,// S
  0x01, 0x01, 0x7F, 0x01, 0x01,// T
  0x3F, 0x40, 0x40, 0x40, 0x3F,// U
  0x1F, 0x20, 0x40, 0x20, 0x1F,// V
  0x7F, 0x20, 0x18, 0x20, 0x7F,// W
  0x63, 0x14, 0x08, 0x14, 0x63,// X
  0x03, 0x04, 0x78, 0x04, 0x03,// Y
  0x61, 0x51, 0x49, 0x45, 0x43,// Z
  0x00, 0x00, 0x7F, 0x41, 0x41,// [
  0x02, 0x04, 0x08, 0x10, 0x20,// "\"
  0x41, 0x41, 0x7F, 0x00, 0x00,// ]
  0x04, 0x02, 0x01, 0x02, 0x04,// ^
  0x40, 0x40, 0x40, 0x40, 0x40,// _
  0x00, 0x01, 0x02, 0x04, 0x00,// `
  0x20, 0x54, 0x54, 0x54, 0x78,// a
  0x7F, 0x48, 0x44, 0x44, 0x38,// b
  0x38, 0x44, 0x44, 0x44, 0x20,// c
  0x38, 0x44, 0x44, 0x48, 0x7F,// d
  0x38, 0x54, 0x54, 0x54, 0x18,// e
  0x08, 0x7E, 0x09, 0x01, 0x02,// f
  0x08, 0x14, 0x54, 0x54, 0x3C,// g
  0x7F, 0x08, 0x04, 0x04, 0x78,// h
  0x00, 0x44, 0x7D, 0x40, 0x00,// i
  0x20, 0x40, 0x44, 0x3D, 0x00,// j
  0x00, 0x7F, 0x10, 0x28, 0x44,// k
  0x00, 0x41, 0x7F, 0x40, 0x00,// l
  0x7C, 0x04, 0x18, 0x04, 0x78,// m
  0x7C, 0x08, 0x04, 0x04, 0x78,// n
  0x38, 0x44, 0x44, 0x44, 0x38,// o
  0x7C, 0x14, 0x14, 0x14, 0x08,// p
  0x08, 0x14, 0x14, 0x18, 0x7C,// q
  0x7C, 0x08, 0x04, 0x04, 0x08,// r
  0x48, 0x54, 0x54, 0x54, 0x20,// s
  0x04, 0x3F, 0x44, 0x40, 0x20,// t
  0x3C, 0x40, 0x40, 0x20, 0x7C,// u
  0x1C, 0x20, 0x40, 0x20, 0x1C,// v
  0x3C, 0x40, 0x30, 0x40, 0x3C,// w
  0x44, 0x28, 0x10, 0x28, 0x44,// x
  0x0C, 0x50, 0x50, 0x50, 0x3C,// y
  0x44, 0x64, 0x54, 0x4C, 0x44,// z
  0x00, 0x08, 0x36, 0x41, 0x00,// {
  0x00, 0x00, 0x7F, 0x00, 0x00,// |
  0x00, 0x41, 0x36, 0x08, 0x00,// }
  0x08, 0x08, 0x2A, 0x1C, 0x08,// ->
  0x08, 0x1C, 0x2A, 0x08, 0x08 // <-
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

uint16_t XYsafe( int x, int y)
{
  if ( x >= kMatrixWidth) return -1;
  if ( y >= kMatrixHeight) return -1;
  if ( x < 0) return -1;
  if ( y < 0) return -1;
  return XY(x, y);
}

inline boolean between(int l, int x, int u) {
  return (unsigned)(x - l) <= (u - l);
}

int air_below(int stone_idx, int pos_x, int pos_y, int rotation) {
  int count = 0;
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
      for( int y = pixel_y-1; y >= 0; y-- ) {
        if( board[pixel_x][y] == 0 ) {
          count++;
        } else {
          break;
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

void loop()
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
  if( ms - last_stone_move > (button_left ? FAST_MOVE_TIME : (SLOW_MOVE_TIME - (level*SLOW_MOVE_STEP) )/(demo_mode ? 4 : 1) ) ) {
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

int _mod(int x, int m) {
  int r = x%m;
  return r<0 ? r+m : r;
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

void Clear() {
  for ( byte y = 0; y < kMatrixHeight; y++) {
    for ( byte x = 0; x < kMatrixWidth; x++) {
      leds[ XYsafe(x, y)]  = CHSV((16*y)+(47*x), 255, 42);
    }
  }
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

void button_right() {
  uint32_t ms = millis();
  if( ms > oldmillis+PRELLBOCKZEIT) {
    demo_mode = false;
    rotate();
    oldmillis = ms;
  }
}

unsigned char* Char(unsigned char* font, char c) {
  return &font[(c - 32) * 5];
}

void DrawText(char *text, int x, int y) {
  for (int i = 0; i < strlen(text); i++) {
    DrawSprite(Char(Font5x7, text[i]), 5, x + i * 6, y);
  }
}

void DrawTextOneFrame(char *text, int xOffset) {
  DrawText(text, 10 - xOffset, 0);
}

void DrawNumberOneFrame(uint32_t number, int xOffset) {
  char buffer[7];
  itoa(number,buffer,10);
  DrawTextOneFrame(buffer, xOffset); 
}

void DrawSprite(unsigned char* sprite, int length, int xOffset, int yOffset) {
  //  leds[ XYsafe(x, y) ] = CHSV(0, 0, 255);
  for ( byte y = 0; y < 7; y++) {
    for ( byte x = 0; x < 5; x++) {
      bool on = (sprite[x] >> (6 - y) & 1) * 255;
      if (on) {
        leds[ XYsafe(x + xOffset, y + yOffset)]  = CHSV(44, 255, 255);
      }
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
      CRGB p = leds[ XY(x,y) ];
      if( p.r == 0 && p.g == 0 && p.b == 0 ) {
        leds[ XY(x, y)]  = CHSV( pixelHue, 255, 48);
      }
    }
  }
}

void setup() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness( BRIGHTNESS );
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


















