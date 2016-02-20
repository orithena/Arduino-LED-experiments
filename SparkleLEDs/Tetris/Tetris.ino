#include <FastLED.h>

//#define DEBUG
#define LED_PIN               6            // LED chain data pin
#define COLOR_ORDER RGB                    // LED chip color order
#define CHIPSET     WS2811                 // LED chipset
#define BRIGHTNESS 255                     // LED overall brightness

#define IN_ANALOGRIGHT_PIN   A3
#define IN_ANALOGLEFT_PIN    A4
#define IN_ANALOGLEFT_LOWER_BOUND    298   // min raw value of left analog controller
#define IN_ANALOGLEFT_UPPER_BOUND    744   // max raw value of left analog controller
#define IN_ANALOGRIGHT_LOWER_BOUND   235   // min raw value of right analog controller
#define IN_ANALOGRIGHT_UPPER_BOUND   678   // max raw value of right analog controller
#define ANALOG_JITTER         24           // not counting analog changes below that threshold

#define IN_BUTTONLEFT_PIN     2            // left digital button pin
#define IN_BUTTONRIGHT_PIN    3            // right digital button pin
#define PRELLBOCKZEIT       168            // buttons cannot be counted as pressed twice in this time frame  

#define SLOW_MOVE_TIME  1500
#define SLOW_MOVE_STEP    75
#define FAST_MOVE_TIME    75

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
      _BA, _BB, _CA, _CB     }
    ,
    { 
      _BA, _BB, _CA, _CB     }
    ,
    { 
      _BA, _BB, _CA, _CB     }
    ,
    { 
      _BA, _BB, _CA, _CB     }
  }
  , { // 1 = el block
    { 
      _AA, _AB, _BB, _CB     }
    ,
    { 
      _AC, _BA, _BB, _BC     }
    ,
    { 
      _AA, _BA, _CA, _CB     }
    ,
    { 
      _BA, _BB, _BC, _CA     }
  }
  , { // 2 = reverse squiggely
    { 
      _AB, _BB, _BA, _CA     }
    ,
    { 
      _AA, _AB, _BB, _BC     }
    ,
    { 
      _AB, _BB, _BA, _CA     }
    ,
    { 
      _AA, _AB, _BB, _BC     }
  }
  , { // 3 = reverse el block
    { 
      _AA, _AB, _BA, _CA     }
    ,
    { 
      _BA, _BB, _BC, _CC     }
    ,
    { 
      _AB, _BB, _CB, _CA     }
    ,
    { 
      _AA, _BA, _BB, _BC     }
  }
  , { // 4 = T block
    { 
      _AB, _BB, _BA, _CB     }
    ,
    { 
      _AB, _BB, _BA, _BC     }
    ,
    { 
      _AA, _BB, _BA, _CA     }
    ,
    { 
      _BA, _BB, _BC, _CB     }
  }
  , { // 5 = squiggely
    { 
      _AA, _BA, _BB, _CB     }
    ,
    { 
      _BC, _BB, _CB, _CA     }
    ,
    { 
      _AA, _BA, _BB, _CB     }
    ,
    { 
      _BC, _BB, _CB, _CA     }
  }
  , { // 6 = LINE PIECE!!!
    { 
      _BA, _BB, _BC, _BD     }
    ,
    { 
      _AA, _BA, _CA, _DA     }
    ,
    { 
      _BA, _BB, _BC, _BD     }
    ,
    { 
      _AA, _BA, _CA, _DA     }
  }
};

// internal state variables
volatile byte stone = 6;
volatile byte rotation = 0;
int analog_left, analog_left_raw_old = 0;
int analog_right, analog_right_raw_old = 0;
int stone_x = kBoardWidth/2;
int stone_y = kBoardHeight;
volatile uint32_t oldmillis = 0;
uint32_t last_stone_move = 0;

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

boolean possible(int pos_x, int pos_y, int rotation) {
  for( int p = 0; p < 4; p++ ) {
    if( !between(0, pos_x + stones[stone][rotation][p][0], kBoardWidth-1 ) ) return false;
    if( pos_y + stones[stone][rotation][p][1] < 0 ) return false;
    if( between(0, pos_x + stones[stone][rotation][p][0], kBoardWidth-1  ) &&
      between(0, pos_y + stones[stone][rotation][p][1], kBoardHeight-1 ) ) {
      if( board[ pos_x + stones[stone][rotation][p][0] ][ pos_y + stones[stone][rotation][p][1] ] > 0 ) {
        return false;
      }
    }
  }
  return true;
}

void blink_line(int y, int repeat) {
  for( int n = 0; n < repeat; n++ ) {
    for( int x = 0; x < kMatrixWidth; x++ ) {
      leds[ XYsafe( x,y ) ] = CHSV( ((n*kMatrixWidth)+(x*36)+(y*36L*(repeat%8)))%256, 255, 128 );
    }
    FastLED.show();
    delay(33);
  }
}

void blank_line(int y, int pause) {
  for( int x = 0; x < kMatrixWidth; x++ ) {
    leds[ XYsafe( x,y ) ] = CHSV( 0,0,0 );
  }
  FastLED.show();
  delay(pause);
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
  int analog_right_raw = analogRead(IN_ANALOGRIGHT_PIN);
  int analog_left_raw = analogRead(IN_ANALOGLEFT_PIN);
  if( abs(analog_left_raw_old - analog_left_raw) > ANALOG_JITTER ) {
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
  if( abs(analog_right_raw_old - analog_right_raw) > ANALOG_JITTER ) {
    if( analog_right_raw > IN_ANALOGRIGHT_UPPER_BOUND ) analog_right_raw = IN_ANALOGRIGHT_UPPER_BOUND;
    if( analog_right_raw < IN_ANALOGRIGHT_LOWER_BOUND ) analog_right_raw = IN_ANALOGRIGHT_LOWER_BOUND;
    analog_right = ((analog_right_raw - IN_ANALOGRIGHT_LOWER_BOUND) * 255L) / (IN_ANALOGRIGHT_UPPER_BOUND - IN_ANALOGRIGHT_LOWER_BOUND);
    analog_right_raw_old = analog_right_raw;
#ifdef DEBUG
    Serial.print("right ");
    Serial.print(analog_right);
    Serial.print(" ");
    Serial.println(analog_right_raw_old);
#endif
  }
}

void loop()
{
  uint32_t ms = millis();
  readController();

  int board_offset = ((analog_left * (kBoardHeight - kMatrixHeight)) / 240);

  int new_x = kBoardWidth - 1 - ((analog_right * kBoardWidth) / 256);
  for( int x = stone_x; (stone_x < new_x ? x <= new_x : x >= new_x); x = (stone_x < new_x ? x+1 : x-1) ) {
    if( possible(x, stone_y, rotation) ) {
      stone_x = x;
    } 
    else {
      break;
    }
  }

  boolean button_left = (digitalRead(IN_BUTTONLEFT_PIN) == HIGH);
  if( ms - last_stone_move > (button_left ? FAST_MOVE_TIME : SLOW_MOVE_TIME ) ) {
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
    } 
    else { 
      // stone cannot go down, fix it to board!
      for( int p = 0; p < 4; p++ ) {
        if( between(0, stone_x + stones[stone][rotation][p][0], kBoardWidth-1) &&
          between(0, stone_y + stones[stone][rotation][p][1], kBoardHeight-1) ) {
          board[ stone_x + stones[stone][rotation][p][0] ][ stone_y + stones[stone][rotation][p][1] ] = stone + 1;
        }
      }

      // check for full lines
      for( int y = kBoardHeight-1; y >= 0; y-- ) {
        boolean line_full = true;
        for( int x = 0; x < kBoardWidth; x++ ) {
          if( board[x][y] == 0 ) line_full = false;
        }
        if( line_full ) {
          if( y - board_offset < kMatrixHeight && y - board_offset >= 0 ) {
            blink_line(y, 96);
            blank_line(y,100);
          }
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
        for( int y = kBoardHeight - 1; y >= 0; y-- ) {
          blink_line(y, 18);
          blank_line(y, 0);
          destroy_line(y);
        }
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
  delay(1);
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
      if( to > 0 ) {
        leds[ XYsafe(x,y) ] = palette(to);
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
            leds[ XYsafe(x,base_line + y) ] = CHSV(0, 0, 128);
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

void button_right() {
  uint32_t ms = millis();
  if( ms > oldmillis+PRELLBOCKZEIT) {
    // lets try rotating that stone.
    if( possible( stone_x, stone_y, (rotation + 1) % 4 ) ) {
      rotation = (rotation + 1) % 4;
    } // we might be at the right wall. try one position to the left.
    else if( possible( stone_x-1, stone_y, (rotation + 1) % 4 ) ) {
      stone_x -= 1;
      rotation = (rotation + 1) % 4;
    } // nope? might be the left wall. try one position to the right.
    else if( possible( stone_x+1, stone_y, (rotation + 1) % 4 ) ) {
      stone_x += 1;
      rotation = (rotation + 1) % 4;
    } // okay, we've still got the long stick that might want to move two positions to the left.
    else if( possible( stone_x-2, stone_y, (rotation + 1) % 4 ) ) {
      stone_x -= 2;
      rotation = (rotation + 1) % 4;
    } // I give up and leave it at that.
    oldmillis = ms;
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
}









